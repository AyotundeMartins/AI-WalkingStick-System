#include <DHT.h>
#include <SoftwareSerial.h>
#include <TinyGPS++.h>
#include <DFRobotDFPlayerMini.h>

// ================================
// PIN CONFIGURATION
// ================================
const int irPin = A0;           // IR sensor output
const int vibrationPin = A1;    // Vibration motor (positive leg)
const int batMotPin = A2;       // Battery motor
const int rainAnalogPin = A4;   // Rain sensor analog (Ao)
const int rainDigitalPin = A5;  // Rain sensor digital (Do)
const int panicButtonPin = A6;  // Panic button
const int extralPin = A7;       // Extra pin
const int echoPin = A14;        // Ultrasonic echo
const int trigPin = A15;        // Ultrasonic trigger
const int statusLED = 13;       // Status LED

// DHT22 Configuration
#define DHTPIN A3
#define DHTTYPE DHT22
DHT dht(DHTPIN, DHTTYPE);

// Serial Communication Setup
SoftwareSerial dfSerial(A9, A8);    // DF Mini: RX, TX (Arduino pins)
SoftwareSerial simSerial(A11, A10);  // GSM: RX, TX
SoftwareSerial gpsSerial(A12, A13);  // GPS: RX, TX
DFRobotDFPlayerMini dfPlayer;
TinyGPSPlus gps;

// Audio and Vibration Status
bool audioInitialized = false;
bool vibrationEnabled = true;
bool isPlayingAudio = false;        // NEW: Track audio playback state
unsigned long audioStartTime = 0;   // NEW: Track when audio started
int currentPlayingTrack = -1;       // NEW: Track current audio file

// ================================
// SYSTEM CONFIGURATION
// ================================
const String EMERGENCY_PHONE = "+2348164566201";
const String DEVICE_ID = "WalkingStick_001";

// AI System Flags
#define VOICE_ENABLED true
#define SMART_ALERTS true
#define LEARNING_MODE true

// ================================
// TIMING CONSTANTS
// ================================
const unsigned long SENSOR_INTERVAL = 2000;    // 2 seconds
const unsigned long DHT_INTERVAL = 3000;       // 3 seconds
const unsigned long GPS_TIMEOUT = 10000;       // 10 seconds
const unsigned long ALERT_COOLDOWN = 3000;     // 3 seconds between alerts
const unsigned long VOICE_COOLDOWN = 5000;     // 5 seconds between voice
const unsigned long DEBOUNCE_DELAY = 50;       // Button debounce
const unsigned long AUDIO_TIMEOUT = 10000;     // NEW: Max audio duration (10 seconds)

// ================================
// AI DETECTION THRESHOLDS
// ================================
const float DISTANCE_THRESHOLD = 100.0;        // cm for large objects
const int IR_THRESHOLD = 600;                  // Small object detection
const int RAIN_THRESHOLD = 500;                // Wet surface detection

// Rain Level Thresholds
const int RAIN_DRY = 1000;
const int RAIN_LIGHT = 700;
const int RAIN_MODERATE = 400;
const int RAIN_HEAVY = 200;

// Environmental Comfort Zones
const float TEMP_MIN_COMFORT = 15.0;
const float TEMP_MAX_COMFORT = 35.0;
const float HUMIDITY_MIN_COMFORT = 30.0;
const float HUMIDITY_MAX_COMFORT = 80.0;

// DHT22 Calibration
const float TEMP_OFFSET = 0.0;
const float TEMP_MIN_VALID = 15.0;
const float TEMP_MAX_VALID = 40.0;

// ================================
// AUDIO TRACK DEFINITIONS
// ================================
const int TRACK_SYSTEM_READY = 1;
const int TRACK_LARGE_OBSTACLE = 2;
const int TRACK_SMALL_OBSTACLE = 3;
const int TRACK_WATER_HAZARD = 4;
const int TRACK_PANIC_ALERT = 5;
const int TRACK_PATH_CLEAR = 6;
const int TRACK_RAIN_WARNING = 7;
const int TRACK_COLD_WARNING = 8;
const int TRACK_HOT_WARNING = 9;

// NEW: Audio track durations (in milliseconds)
const int TRACK_DURATIONS[] = {0, 3000, 2500, 2000, 3500, 4000, 2000, 2500, 2500, 2500};

// ================================
// DATA STRUCTURES
// ================================
struct SensorData {
  float temperature;
  float humidity;
  float distance;
  int irValue;
  int rainValue;
  String rainLevel;
  bool gpsValid;
  double latitude;
  double longitude;
  unsigned long timestamp;
};

struct LastKnownLocation {
  double latitude;
  double longitude;
  unsigned long timestamp;
  bool isValid;
};

struct AIState {
  int alertLevel;                    // 0=Normal, 1=Caution, 2=Warning, 3=Critical
  String primaryConcern;
  String obstacleType;
  bool adaptiveMode;
  int consecutiveAlerts;
  unsigned long lastAlertTime;
  unsigned long lastVoiceAnnouncement;
  bool vibrationActive;              // NEW: Track vibration state
  unsigned long vibrationStartTime;  // NEW: Track vibration timing
};

// ================================
// GLOBAL VARIABLES
// ================================
SensorData currentReading;
AIState aiState = { 0, "Normal", "None", true, 0, 0, 0, false, 0 };
LastKnownLocation lastLocation = { 0.0, 0.0, 0, false };

// Timing variables
unsigned long lastSensorRead = 0;
unsigned long lastGPSUpdate = 0;
unsigned long lastDHTRead = 0;

// Button debouncing
unsigned long lastButtonPress = 0;
bool lastButtonState = HIGH;
bool buttonState = HIGH;

// NEW: Vibration control variables
bool vibrationTriggeredBySensor = false;
String lastVibrationSource = "";

// ================================
// SETUP FUNCTION
// ================================
void setup() {
  Serial.begin(9600);
  
  // Initialize sensors and communication
  dht.begin();
  gpsSerial.begin(9600);
  simSerial.begin(9600);
  dfSerial.begin(9600);
  
  // Configure pins
  setupPins();
  
  // Initialize system components
  initializeSystem();
  
  Serial.println("AI Walking Stick Ready!");
  Serial.println("============================================\n");
}

void setupPins() {
  // Input pins
  pinMode(echoPin, INPUT);
  pinMode(irPin, INPUT);
  pinMode(rainDigitalPin, INPUT);
  pinMode(rainAnalogPin, INPUT);
  pinMode(panicButtonPin, INPUT_PULLUP);
  
  // Output pins
  pinMode(trigPin, OUTPUT);
  pinMode(vibrationPin, OUTPUT);
  pinMode(batMotPin, OUTPUT);  // Battery motor pin
  pinMode(statusLED, OUTPUT);
  
  // Initial states
  digitalWrite(vibrationPin, LOW);
  digitalWrite(batMotPin, LOW);
  digitalWrite(statusLED, HIGH);
  
  // Test vibration motor at startup
  Serial.println("Testing vibration motor...");
  testVibrationMotor();
}

void initializeSystem() {
  Serial.println("====== AI WALKING STICK INITIALIZING ======");
  Serial.println("Smart Navigation System for Visually Impaired v4.0");
  
  // Initialize DFPlayer Mini with better error handling
  initializeDFPlayer();
  
  // Test vibration system
  Serial.println("Testing vibration system...");
  vibrationPattern("confirmation", "system");
  delay(1000);
  
  // Initialize SIM module
  initializeSIMModule();
  
  // Wait for DHT stabilization
  delay(3000);
  
  // Test sensors
  testSensorInitialization();
  
  // Optional: Test audio system if initialized
  if (audioInitialized) {
    Serial.println("Playing startup sound...");
    playTrack(TRACK_SYSTEM_READY);
    delay(4000);
  }
  
  // Final system status
  Serial.println("\n=== SYSTEM STATUS ===");
  Serial.println("Audio System: " + String(audioInitialized ? "OK" : "DISABLED"));
  Serial.println("Vibration: " + String(vibrationEnabled ? "OK" : "DISABLED"));
  Serial.println("====================");
}

void initializeDFPlayer() {
  Serial.println("Initializing DFPlayer Mini...");
  Serial.println("Checking connections: TX->A8, RX->A9, VCC->5V, GND->GND");
  
  // Give DFPlayer time to initialize
  delay(3000);
  
  if (!dfPlayer.begin(dfSerial, true, true)) {  // Use acknowledgment
    Serial.println("DFPlayer Mini initialization failed!");
    Serial.println("Troubleshooting steps:");
    Serial.println("1. Check wiring: TX->A8, RX->A9");
    Serial.println("2. Verify SD card is inserted");
    Serial.println("3. Check SD card format (FAT16/FAT32)");
    Serial.println("4. Ensure audio files are named 0001.mp3, 0002.mp3, etc.");
    Serial.println("5. Check power supply (5V)");
    audioInitialized = false;
  } else {
    Serial.println("DFPlayer Mini online!");
    
    // Configure DFPlayer settings
    delay(1000);
    dfPlayer.setTimeOut(500);
    
    // Set volume (0-30)
    dfPlayer.volume(20);
    delay(500);
    
    // Set equalizer
    dfPlayer.EQ(DFPLAYER_EQ_NORMAL);
    delay(500);
    
    // Check if SD card is present
    int fileCount = dfPlayer.readFileCounts();
    delay(500);
    
    if (fileCount > 0) {
      Serial.println("SD card detected with " + String(fileCount) + " files");
      audioInitialized = true;
      
      // Test audio with a simple beep
      Serial.println("Testing audio...");
      dfPlayer.play(1);  // Play first track
      delay(3000);
      
    } else {
      Serial.println("No audio files found on SD card!");
      Serial.println("Add files named: 0001.mp3, 0002.mp3, etc.");
      audioInitialized = false;
    }
  }
  
  if (!audioInitialized) {
    Serial.println("Audio system disabled - continuing without voice guidance");
  }
}

// ================================
// MAIN LOOP (IMPROVED)
// ================================
void loop() {
  unsigned long currentTime = millis();
  
  // Monitor and manage audio playback
  manageAudioPlayback();
  
  // Monitor and manage vibration
  manageVibration();
  
  // Collect sensor data at specified intervals
  if (currentTime - lastSensorRead >= SENSOR_INTERVAL) {
    collectAllSensorData();
    performAIAnalysis();
    handleIntelligentAlerts();
    logAISensorData();
    lastSensorRead = currentTime;
  }
  
  // Process GPS data continuously
  processGPS();
  
  // Handle panic button with debouncing
  handlePanicButton();
  
  delay(50);  // Small delay to prevent overwhelming the system
}

// ================================
// NEW: AUDIO MANAGEMENT SYSTEM
// ================================
void manageAudioPlayback() {
  if (isPlayingAudio) {
    unsigned long currentTime = millis();
    
    // Check if audio should have finished based on track duration
    int expectedDuration = TRACK_DURATIONS[currentPlayingTrack];
    if (currentTime - audioStartTime > expectedDuration || 
        currentTime - audioStartTime > AUDIO_TIMEOUT) {
      
      Serial.println("Audio playback completed/timed out for track " + String(currentPlayingTrack));
      stopAudio();
    }
    
    // Also check DFPlayer status if available
    if (audioInitialized && dfPlayer.available()) {
      uint8_t type = dfPlayer.readType();
      int value = dfPlayer.read();
      
      if (type == DFPlayerPlayFinished) {
        Serial.println("Audio finished playing track: " + String(value));
        stopAudio();
      } else if (type == DFPlayerError) {
        Serial.println("Audio error occurred: " + String(value));
        stopAudio();
      }
    }
  }
}

// ================================
// NEW: VIBRATION MANAGEMENT SYSTEM
// ================================
void manageVibration() {
  if (aiState.vibrationActive) {
    unsigned long currentTime = millis();
    
    // Auto-stop vibration after maximum duration (safety feature)
    if (currentTime - aiState.vibrationStartTime > 5000) {  // Max 5 seconds
      digitalWrite(vibrationPin, LOW);
      aiState.vibrationActive = false;
      Serial.println("Vibration auto-stopped (safety timeout)");
    }
  }
}

// ================================
// SENSOR DATA COLLECTION
// ================================
void collectAllSensorData() {
  currentReading.timestamp = millis();
  
  // Ultrasonic distance sensor
  currentReading.distance = readUltrasonicDistance();
  
  // IR proximity sensor
  currentReading.irValue = analogRead(irPin);
  
  // DHT22 temperature and humidity
  readDHTSensor();
  
  // Rain sensor
  readRainSensor();
  
  // GPS data
  updateGPSData();
}

float readUltrasonicDistance() {
  digitalWrite(trigPin, LOW);
  delayMicroseconds(2);
  digitalWrite(trigPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigPin, LOW);
  
  long duration = pulseIn(echoPin, HIGH, 30000);
  
  if (duration == 0) return -1;  // No echo received
  
  float distance = duration * 0.034 / 2;
  
  // Validate distance range
  if (distance < 2 || distance > 400) return -1;
  
  return distance;
}

void readDHTSensor() {
  if (millis() - lastDHTRead >= DHT_INTERVAL) {
    currentReading.humidity = dht.readHumidity();
    delay(250);
    
    // Multiple attempts for temperature reading
    for (int attempt = 0; attempt < 3; attempt++) {
      float temp = dht.readTemperature();
      if (!isnan(temp)) {
        currentReading.temperature = temp + TEMP_OFFSET;
        if (currentReading.temperature >= TEMP_MIN_VALID && 
            currentReading.temperature <= TEMP_MAX_VALID) {
          break;
        }
      }
      delay(500);
    }
    lastDHTRead = millis();
  }
}

void readRainSensor() {
  int rainDigital = digitalRead(rainDigitalPin);
  currentReading.rainValue = analogRead(rainAnalogPin);
  currentReading.rainLevel = getRainLevel(currentReading.rainValue);
}

void updateGPSData() {
  currentReading.gpsValid = gps.location.isValid();
  if (currentReading.gpsValid) {
    currentReading.latitude = gps.location.lat();
    currentReading.longitude = gps.location.lng();
    
    // Update last known location
    lastLocation.latitude = currentReading.latitude;
    lastLocation.longitude = currentReading.longitude;
    lastLocation.timestamp = millis();
    lastLocation.isValid = true;
  }
}

String getRainLevel(int analogValue) {
  if (analogValue >= RAIN_DRY) return "Dry";
  else if (analogValue >= RAIN_LIGHT) return "Light";
  else if (analogValue >= RAIN_MODERATE) return "Moderate";
  else if (analogValue >= RAIN_HEAVY) return "Heavy";
  else return "Very Heavy";
}

// ================================
// AI ANALYSIS SYSTEM
// ================================
void performAIAnalysis() {
  // Reset AI state
  aiState.alertLevel = 0;
  aiState.primaryConcern = "Normal";
  aiState.obstacleType = "None";
  
  // Multi-factor analysis
  analyzeObstacleThreats();
  analyzeEnvironmentalSafety();
  analyzeSurfaceConditions();
  
  // Adaptive learning
  if (LEARNING_MODE) {
    adaptiveThresholdAdjustment();
  }
}

void analyzeObstacleThreats() {
  aiState.obstacleType = classifyObstacle(currentReading.distance,
                                          currentReading.irValue,
                                          currentReading.rainValue);
  
  if (aiState.obstacleType != "None") {
    if (aiState.obstacleType.indexOf("Water") >= 0) {
      aiState.alertLevel = max(aiState.alertLevel, 3);  // Critical
      aiState.primaryConcern = "Water Hazard Detected";
    } else if (aiState.obstacleType.indexOf("Large") >= 0) {
      aiState.alertLevel = max(aiState.alertLevel, 2);  // Warning
      aiState.primaryConcern = "Large Obstacle Ahead";
    } else if (aiState.obstacleType.indexOf("Small") >= 0) {
      aiState.alertLevel = max(aiState.alertLevel, 1);  // Caution
      aiState.primaryConcern = "Small Object Detected";
    }
  }
}

void analyzeEnvironmentalSafety() {
  // Temperature analysis
  if (!isnan(currentReading.temperature)) {
    if (currentReading.temperature < TEMP_MIN_COMFORT) {
      aiState.alertLevel = max(aiState.alertLevel, 1);
      if (aiState.primaryConcern == "Normal") {
        aiState.primaryConcern = "Cold Environment";
      }
    } else if (currentReading.temperature > TEMP_MAX_COMFORT) {
      aiState.alertLevel = max(aiState.alertLevel, 1);
      if (aiState.primaryConcern == "Normal") {
        aiState.primaryConcern = "Hot Environment";
      }
    }
  }
  
  // Humidity analysis
  if (!isnan(currentReading.humidity)) {
    if (currentReading.humidity > HUMIDITY_MAX_COMFORT) {
      aiState.alertLevel = max(aiState.alertLevel, 1);
      if (aiState.primaryConcern == "Normal") {
        aiState.primaryConcern = "High Humidity";
      }
    }
  }
}

void analyzeSurfaceConditions() {
  if (currentReading.rainLevel != "Dry") {
    if (currentReading.rainLevel == "Very Heavy" || currentReading.rainLevel == "Heavy") {
      aiState.alertLevel = max(aiState.alertLevel, 2);
      aiState.primaryConcern = "Heavy Rain - Slip Risk";
    } else if (currentReading.rainLevel == "Moderate") {
      aiState.alertLevel = max(aiState.alertLevel, 1);
      if (aiState.primaryConcern == "Normal") {
        aiState.primaryConcern = "Wet Surface Caution";
      }
    }
  }
}

String classifyObstacle(float distance, int irVal, int rainVal) {
  // Large object detection
  if (distance > 0 && distance < DISTANCE_THRESHOLD) {
    return "Large object ahead";
  }
  
  // Small object detection
  if (irVal > IR_THRESHOLD) {
    return "Small object ahead";
  }
  
  // Water/liquid hazard detection
  if (rainVal < RAIN_THRESHOLD) {
    return "Water detected";
  }
  
  return "None";
}

void adaptiveThresholdAdjustment() {
  if (aiState.consecutiveAlerts > 8) {
    Serial.println("AI: Adaptive mode - adjusting sensitivity");
    aiState.consecutiveAlerts = 0;
  }
}

// ================================
// IMPROVED ALERT AND GUIDANCE SYSTEM
// ================================
void handleIntelligentAlerts() {
  unsigned long currentTime = millis();
  
  if (aiState.alertLevel > 0 && (currentTime - aiState.lastAlertTime) > ALERT_COOLDOWN) {
    // Only trigger vibration for sensor-related alerts
    if (shouldTriggerVibrationForSensors()) {
      // Vibration patterns based on severity
      switch (aiState.alertLevel) {
        case 1: vibrationPattern("caution", "sensor"); break;
        case 2: vibrationPattern("warning", "sensor"); break;
        case 3: vibrationPattern("critical", "sensor"); break;
      }
    }
    
    // Voice guidance (only if not already playing audio)
    if (VOICE_ENABLED && !isPlayingAudio && 
        (currentTime - aiState.lastVoiceAnnouncement) > VOICE_COOLDOWN) {
      announceIntelligentGuidance();
      aiState.lastVoiceAnnouncement = currentTime;
    }
    
    aiState.lastAlertTime = currentTime;
    aiState.consecutiveAlerts++;
  } else if (aiState.alertLevel == 0) {
    // Reset on normal conditions
    aiState.consecutiveAlerts = max(0, aiState.consecutiveAlerts - 1);
    
    // Stop sensor-triggered vibration
    if (vibrationTriggeredBySensor) {
      digitalWrite(vibrationPin, LOW);
      aiState.vibrationActive = false;
      vibrationTriggeredBySensor = false;
    }
    
    // Occasional path clear announcement (only if not playing audio)
    if (!isPlayingAudio && (currentTime - aiState.lastVoiceAnnouncement) > VOICE_COOLDOWN * 2) {
      playTrack(TRACK_PATH_CLEAR);
      aiState.lastVoiceAnnouncement = currentTime;
    }
  }
}

// NEW: Check if vibration should be triggered by sensors
bool shouldTriggerVibrationForSensors() {
  // Only respond to obstacle-related alerts
  return (aiState.obstacleType != "None") || 
         (aiState.primaryConcern.indexOf("Hazard") >= 0) ||
         (aiState.primaryConcern.indexOf("Rain") >= 0);
}

void announceIntelligentGuidance() {
  Serial.print("AI VOICE GUIDANCE: ");
  
  if (aiState.obstacleType.indexOf("Large") >= 0) {
    Serial.println("Large obstacle detected ahead");
    playTrack(TRACK_LARGE_OBSTACLE);
  } else if (aiState.obstacleType.indexOf("Small") >= 0) {
    Serial.println("Small object detected");
    playTrack(TRACK_SMALL_OBSTACLE);
  } else if (aiState.obstacleType.indexOf("Water") >= 0) {
    Serial.println("Water hazard - danger!");
    playTrack(TRACK_WATER_HAZARD);
  } else if (aiState.primaryConcern.indexOf("Rain") >= 0) {
    Serial.println("Wet surface detected - caution");
    playTrack(TRACK_RAIN_WARNING);
  } else if (aiState.primaryConcern.indexOf("Cold") >= 0) {
    Serial.println("Cold environment warning");
    playTrack(TRACK_COLD_WARNING);
  } else if (aiState.primaryConcern.indexOf("Hot") >= 0) {
    Serial.println("High temperature warning");
    playTrack(TRACK_HOT_WARNING);
  }
}

// ================================
// IMPROVED VIBRATION CONTROL
// ================================
void testVibrationMotor() {
  Serial.println("Testing vibration motor on pin A1...");
  
  // Test basic vibration
  for (int i = 0; i < 3; i++) {
    digitalWrite(vibrationPin, HIGH);
    aiState.vibrationActive = true;
    aiState.vibrationStartTime = millis();
    Serial.println("Vibration ON");
    delay(500);
    digitalWrite(vibrationPin, LOW);
    aiState.vibrationActive = false;
    Serial.println("Vibration OFF");
    delay(500);
  }
  
  Serial.println("Vibration motor test complete");
}

void vibrationPattern(String patternType, String source) {
  if (!vibrationEnabled) {
    Serial.println("Vibration disabled");
    return;
  }
  
  // Stop any existing vibration first
  digitalWrite(vibrationPin, LOW);
  aiState.vibrationActive = false;
  
  Serial.println("Vibration pattern: " + patternType + " (Source: " + source + ")");
  
  // Track the source of vibration
  lastVibrationSource = source;
  if (source == "sensor") {
    vibrationTriggeredBySensor = true;
  } else {
    vibrationTriggeredBySensor = false;
  }
  
  aiState.vibrationStartTime = millis();
  aiState.vibrationActive = true;
  
  if (patternType == "caution") {
    // Single long buzz for caution
    digitalWrite(vibrationPin, HIGH);
    delay(800);
    digitalWrite(vibrationPin, LOW);
    delay(200);
  } 
  else if (patternType == "warning") {
    // Double buzz for warning
    for (int i = 0; i < 2; i++) {
      digitalWrite(vibrationPin, HIGH);
      delay(400);
      digitalWrite(vibrationPin, LOW);
      delay(300);
    }
  } 
  else if (patternType == "critical") {
    // Triple buzz for critical
    for (int i = 0; i < 3; i++) {
      digitalWrite(vibrationPin, HIGH);
      delay(300);
      digitalWrite(vibrationPin, LOW);
      delay(200);
    }
  } 
  else if (patternType == "emergency") {
    // Rapid emergency pattern
    for (int i = 0; i < 6; i++) {
      digitalWrite(vibrationPin, HIGH);
      delay(150);
      digitalWrite(vibrationPin, LOW);
      delay(100);
    }
  } 
  else if (patternType == "confirmation") {
    // Short confirmation pattern
    digitalWrite(vibrationPin, HIGH);
    delay(200);
    digitalWrite(vibrationPin, LOW);
    delay(100);
    digitalWrite(vibrationPin, HIGH);
    delay(200);
    digitalWrite(vibrationPin, LOW);
  }
  
  // Ensure vibration is off at the end
  digitalWrite(vibrationPin, LOW);
  aiState.vibrationActive = false;
}

void buzzVibration(int times, String source = "manual") {
  if (!vibrationEnabled) return;
  
  lastVibrationSource = source;
  vibrationTriggeredBySensor = (source == "sensor");
  
  for (int i = 0; i < times; i++) {
    digitalWrite(vibrationPin, HIGH);
    aiState.vibrationActive = true;
    aiState.vibrationStartTime = millis();
    delay(400);
    digitalWrite(vibrationPin, LOW);
    aiState.vibrationActive = false;
    delay(300);
  }
}

// ================================
// IMPROVED AUDIO FUNCTIONS
// ================================
void playTrack(int trackNumber) {
  if (!audioInitialized) {
    Serial.println("Audio not available - track " + String(trackNumber) + " requested");
    return;
  }
  
  if (!VOICE_ENABLED) {
    Serial.println("Voice guidance disabled");
    return;
  }
  
  // Don't interrupt audio that's already playing unless it's critical
  if (isPlayingAudio && trackNumber != TRACK_PANIC_ALERT && trackNumber != TRACK_WATER_HAZARD) {
    Serial.println("Audio already playing - skipping track " + String(trackNumber));
    return;
  }
  
  Serial.println("Playing audio track: " + String(trackNumber));
  
  // Stop any currently playing track
  if (isPlayingAudio) {
    dfPlayer.stop();
    delay(200);
  }
  
  // Play the requested track
  dfPlayer.play(trackNumber);
  delay(300);  // Give time for playback to start
  
  // Update audio state
  isPlayingAudio = true;
  currentPlayingTrack = trackNumber;
  audioStartTime = millis();
  
  Serial.println("Audio playback started for track " + String(trackNumber));
}

void setVolume(int volume) {
  if (!audioInitialized) return;
  
  // Constrain volume to valid range (0-30)
  volume = constrain(volume, 0, 30);
  dfPlayer.volume(volume);
  delay(100);
  Serial.println("Volume set to: " + String(volume));
}

void pauseAudio() {
  if (!audioInitialized || !isPlayingAudio) return;
  dfPlayer.pause();
  Serial.println("Audio paused");
}

void resumeAudio() {
  if (!audioInitialized) return;
  dfPlayer.start();
  Serial.println("Audio resumed");
}

void stopAudio() {
  if (!audioInitialized) return;
  
  dfPlayer.stop();
  isPlayingAudio = false;
  currentPlayingTrack = -1;
  audioStartTime = 0;
  
  Serial.println("Audio stopped");
}

// Test audio system with all tracks
void testAudioSystem() {
  if (!audioInitialized) {
    Serial.println("Cannot test audio - system not initialized");
    return;
  }
  
  Serial.println("Testing audio system with all tracks...");
  
  for (int track = 1; track <= 9; track++) {
    Serial.println("Testing track " + String(track));
    playTrack(track);
    
    // Wait for track to finish or timeout
    unsigned long startTime = millis();
    while (isPlayingAudio && (millis() - startTime < 5000)) {
      manageAudioPlayback();
      delay(100);
    }
    
    stopAudio();
    delay(1000);  // Pause between tracks
  }
  
  Serial.println("Audio system test complete");
}

// ================================
// GPS PROCESSING
// ================================
void processGPS() {
  while (gpsSerial.available()) {
    if (gps.encode(gpsSerial.read())) {
      lastGPSUpdate = millis();
    }
  }
}

// ================================
// PANIC BUTTON HANDLING
// ================================
void handlePanicButton() {
  int reading = digitalRead(panicButtonPin);
  
  if (reading != lastButtonState) {
    lastButtonPress = millis();
  }
  
  if ((millis() - lastButtonPress) > DEBOUNCE_DELAY) {
    if (reading != buttonState) {
      buttonState = reading;
      if (buttonState == LOW) {
        handlePanicAlert();
      }
    }
  }
  
  lastButtonState = reading;
}

void handlePanicAlert() {
  Serial.println("\n!!!!! EMERGENCY PANIC ALERT !!!!!");
  
  // Emergency responses - stop any current audio first
  if (isPlayingAudio) {
    stopAudio();
    delay(200);
  }
  
  // Priority emergency actions
  playTrack(TRACK_PANIC_ALERT);
  vibrationPattern("emergency", "panic");
  flashLED();
  
  // Send emergency SMS
  sendEmergencySMS();
  
  // Log emergency report
  logEmergencyReport();
  
  Serial.println("!!!!! EMERGENCY ALERT COMPLETE !!!!!\n");
}

void logEmergencyReport() {
  Serial.println("=== EMERGENCY SITUATION REPORT ===");
  
  // Location information
  if (currentReading.gpsValid) {
    Serial.println("CURRENT LOCATION:");
    Serial.println("Lat: " + String(currentReading.latitude, 6));
    Serial.println("Lon: " + String(currentReading.longitude, 6));
  } else if (lastLocation.isValid) {
    Serial.println("LAST KNOWN LOCATION:");
    Serial.println("Lat: " + String(lastLocation.latitude, 6));
    Serial.println("Lon: " + String(lastLocation.longitude, 6));
    Serial.println("Age: " + String((millis() - lastLocation.timestamp) / 1000) + "s");
  } else {
    Serial.println("NO GPS LOCATION AVAILABLE!");
  }
  
  // AI assessment
  Serial.println("AI Threat Level: " + String(aiState.alertLevel));
  Serial.println("Current Concern: " + aiState.primaryConcern);
  Serial.println("Obstacle Status: " + aiState.obstacleType);
  
  // Environmental conditions
  if (!isnan(currentReading.temperature)) {
    Serial.println("Temperature: " + String(currentReading.temperature) + "°C");
  }
  Serial.println("Surface: " + currentReading.rainLevel);
  
  Serial.println("=== END EMERGENCY REPORT ===");
}

// ================================
// LED FUNCTIONS
// ================================
void flashLED() {
  for (int i = 0; i < 10; i++) {
    digitalWrite(statusLED, !digitalRead(statusLED));
    delay(100);
  }
  digitalWrite(statusLED, HIGH);
}

// ================================
// DATA LOGGING
// ================================
void logAISensorData() {
  Serial.println("====== AI WALKING STICK STATUS ======");
  
  // System status
  Serial.println("Audio Status: " + String(isPlayingAudio ? "Playing Track " + String(currentPlayingTrack) : "Idle"));
  Serial.println("Vibration Status: " + String(aiState.vibrationActive ? "Active (" + lastVibrationSource + ")" : "Idle"));
  
  // Obstacle analysis
  Serial.println("AI Obstacle: " + aiState.obstacleType);
  
  // Distance
  Serial.print("Distance: ");
  if (currentReading.distance > 0) {
    Serial.print(currentReading.distance);
    Serial.print(" cm");
    if (currentReading.distance < DISTANCE_THRESHOLD) Serial.print(" [OBSTACLE]");
  } else {
    Serial.print("Clear/Error");
  }
  Serial.println();
  
  // IR sensor
  Serial.print("IR: " + String(currentReading.irValue));
  if (currentReading.irValue > IR_THRESHOLD) Serial.print(" [DETECTED]");
  Serial.println();
  
  // Environmental data
  if (!isnan(currentReading.temperature)) {
    Serial.println("Temp: " + String(currentReading.temperature) + "°C");
  }
  if (!isnan(currentReading.humidity)) {
    Serial.println("Humidity: " + String(currentReading.humidity) + "%");
  }
  
  // Surface conditions
  Serial.print("Surface: " + currentReading.rainLevel);
  if (currentReading.rainLevel != "Dry") Serial.print(" [WET]");
  Serial.println();
  
  // GPS status
  if (currentReading.gpsValid) {
    Serial.println("GPS: " + String(currentReading.latitude, 6) + ", " + 
                   String(currentReading.longitude, 6));
  } else {
    Serial.println("GPS: Searching...");
  }
  
  // AI assessment
  Serial.println("AI: " + aiState.primaryConcern + " (Level: " + 
                 String(aiState.alertLevel) + ")");
  
  if (aiState.alertLevel > 0) {
    Serial.println("*** GUIDANCE ACTIVE - ALERT TRIGGERED ***");
  } else {
    Serial.println("*** PATH CLEAR - SAFE TO PROCEED ***");
  }
  
  Serial.println("=======================================\n");
}

// ================================
// SIM MODULE FUNCTIONS
// ================================
void initializeSIMModule() {
  Serial.println("Initializing SIM module...");
  delay(2000);
  
  // Test AT command
  simSerial.println("AT");
  delay(1000);
  if (simSerial.available()) {
    String response = simSerial.readString();
    if (response.indexOf("OK") >= 0) {
      Serial.println("SIM module: OK");
    }
  }
  
  // Set SMS text mode
  simSerial.println("AT+CMGF=1");
  delay(1000);
  
  // Check network registration
  simSerial.println("AT+CREG?");
  delay(1000);
  
  Serial.println("SIM module ready");
}

void sendEmergencySMS() {
  Serial.println("Sending emergency SMS...");
  
  // Compose emergency message
  String message = "EMERGENCY ALERT from " + DEVICE_ID + "\n";
  message += "User activated panic button!\n\n";
  
  // Add location if available
  if (currentReading.gpsValid) {
    message += "Location: " + String(currentReading.latitude, 6) + "," + 
               String(currentReading.longitude, 6) + "\n";
    message += "Map: https://maps.google.com/?q=" + String(currentReading.latitude, 6) + 
               "," + String(currentReading.longitude, 6) + "\n\n";
  } else if (lastLocation.isValid) {
    message += "Last Location: " + String(lastLocation.latitude, 6) + "," + 
               String(lastLocation.longitude, 6) + "\n";
    message += "Age: " + String((millis() - lastLocation.timestamp) / 1000) + "s\n\n";
  }
  
  // Add sensor data
  message += "Temp: " + String(currentReading.temperature, 1) + "°C\n";
  message += "Humidity: " + String(currentReading.humidity, 0) + "%\n";
  message += "Surface: " + currentReading.rainLevel + "\n";
  message += "Distance: " + String(currentReading.distance, 1) + "cm\n";
  
  if (aiState.alertLevel > 0) {
    message += "AI Alert: " + aiState.primaryConcern + "\n";
  }
  
  message += "\nImmediate response required!";
  
  // Send SMS
  bool success = sendSMS(EMERGENCY_PHONE, message);
  
  if (success) {
    Serial.println("Emergency SMS sent successfully!");
    vibrationPattern("confirmation", "sms");
  } else {
    Serial.println("Failed to send emergency SMS!");
    vibrationPattern("emergency", "sms_error");
  }
}

bool sendSMS(String phoneNumber, String message) {
  simSerial.println("AT+CMGS=\"" + phoneNumber + "\"");
  delay(1000);
  
  if (simSerial.find(">")) {
    simSerial.print(message);
    delay(100);
    simSerial.write(26);  // Ctrl+Z
    delay(5000);
    
    String response = "";
    unsigned long timeout = millis() + 10000;
    
    while (millis() < timeout) {
      if (simSerial.available()) {
        response += simSerial.readString();
        if (response.indexOf("OK") >= 0) return true;
        if (response.indexOf("ERROR") >= 0) return false;
      }
      delay(100);
    }
  }
  
  return false;
}

// ================================
// UTILITY FUNCTIONS
// ================================
void testSensorInitialization() {
  float testHumidity = dht.readHumidity();
  delay(2000);
  float testTemperature = dht.readTemperature();
  
  if (isnan(testHumidity)) {
    Serial.println("WARNING: DHT22 not responding!");
  } else {
    Serial.println("DHT22: OK");
    if (!isnan(testTemperature)) {
      if (testTemperature < TEMP_MIN_VALID || testTemperature > TEMP_MAX_VALID) {
        Serial.println("WARNING: Temperature out of range!");
      }
    }
  }
}

// ================================
// NEW: ADDITIONAL UTILITY FUNCTIONS
// ================================

// Function to manually stop all outputs (for testing)
void stopAllOutputs() {
  digitalWrite(vibrationPin, LOW);
  aiState.vibrationActive = false;
  vibrationTriggeredBySensor = false;
  
  if (isPlayingAudio) {
    stopAudio();
  }
  
  Serial.println("All outputs stopped manually");
}

// Function to get system status
void printSystemStatus() {
  Serial.println("\n=== SYSTEM STATUS REPORT ===");
  Serial.println("Audio Initialized: " + String(audioInitialized ? "Yes" : "No"));
  Serial.println("Audio Playing: " + String(isPlayingAudio ? "Track " + String(currentPlayingTrack) : "No"));
  Serial.println("Vibration Enabled: " + String(vibrationEnabled ? "Yes" : "No"));
  Serial.println("Vibration Active: " + String(aiState.vibrationActive ? "Yes (" + lastVibrationSource + ")" : "No"));
  Serial.println("Voice Enabled: " + String(VOICE_ENABLED ? "Yes" : "No"));
  Serial.println("Smart Alerts: " + String(SMART_ALERTS ? "Yes" : "No"));
  Serial.println("Learning Mode: " + String(LEARNING_MODE ? "Yes" : "No"));
  Serial.println("Current Alert Level: " + String(aiState.alertLevel));
  Serial.println("Primary Concern: " + aiState.primaryConcern);
  Serial.println("Obstacle Type: " + aiState.obstacleType);
  Serial.println("GPS Valid: " + String(currentReading.gpsValid ? "Yes" : "No"));
  Serial.println("===========================\n");
}

// Function to test all systems sequentially
void runFullSystemTest() {
  Serial.println("\n=== RUNNING FULL SYSTEM TEST ===");
  
  // Test vibration patterns
  Serial.println("Testing vibration patterns...");
  vibrationPattern("confirmation", "test");
  delay(1000);
  vibrationPattern("caution", "test");
  delay(1000);
  vibrationPattern("warning", "test");
  delay(1000);
  vibrationPattern("critical", "test");
  delay(1000);
  
  // Test audio system
  if (audioInitialized) {
    Serial.println("Testing audio system...");
    testAudioSystem();
  }
  
  // Test sensors
  Serial.println("Testing sensors...");
  collectAllSensorData();
  logAISensorData();
  
  // Test GPS
  Serial.println("Testing GPS...");
  processGPS();
  
  Serial.println("=== FULL SYSTEM TEST COMPLETE ===\n");
}