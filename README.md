<b>Accidental Fall Detection System</b>

An IoT-based wearable safety device designed to monitor real-time physical activities, detect accidental falls, and notify caregivers with the user's live GPS location.

---

<b>Authors & Project Members</b>
This is a group project designed and developed by:
<b>Neha Susan Biju</b>,
<b>Neha Fathima.S</b>,
<b>Gayathri.S</b>,
<b>Vani A K</b>.

<b>Academic Guide:</b> Linu Paulose (Assistant Professor, Department of Computer Applications, MITS)  
<b>Institution:</b> Muthoot Institute of Technology and Science (MITS)

---

<b>Project Overview</b>
Accidental falls are a significant source of injury, especially among the elderly and children. This project implements a wearable band that tracks movements in real-time. When a fall occurs, the device:
1. Triggers a local alert using an <b>active buzzer</b> to summon immediate nearby help.
2. Gathers the user's live latitude and longitude via a <b>GPS module</b>.
3. Transmits an <b>SMS alert</b> (containing a Google Maps link) to the caregiver using the <b>Twilio API</b>.
4. Hosts a <b>local web dashboard</b> on port 80 to display system status and coordinates in real-time.

---

<b>Hardware Requirements</b>
The system is built on the following hardware components:
<b>ESP32 DevKit V1: Microcontroller with built-in Wi-Fi & Bluetooth, serving as the central processing unit.</b>
<b>MPU6050 IMU: A 6-axis Inertial Measurement Unit containing a 3-axis accelerometer and a 3-axis gyroscope.</b>
<b>Neo-6M GPS Module: Gathers latitude and longitude coordinates over UART serial communication.</b>
<b>Active Buzzer & LED: Provides local audio-visual feedback in an emergency.</b>
<b>TP4056 Module & Li-ion Battery: Powers the device, enabling wearable form-factor operation.</b>
<b>Breadboard & Jumper Wires: Used for prototyping connections.</b>

---

<b>Repository Structure</b>
```
c:/miniproject/
├── fall_detection_rf/
│   ├── data/
│   │   ├── raw/SisFall/               # Raw SisFall accelerometer/gyroscope text files
│   │   └── processed/                 # CSV files for labeled datasets and extracted features
│   ├── models/
│   │   ├── random_forest_model.pkl    # Trained scikit-learn random forest model
│   │   └── rf_model.h                 # Transpiled C++ header with random forest decision trees
│   ├── src/
│   │   ├── prepare_dataset.py         # Parses raw SisFall dataset into labeled CSV file
│   │   ├── feature_extraction.py      # Groups data into sliding windows & extracts 24 statistical features
│   │   ├── train_rf_model.py          # Trains the scikit-learn RF classifier
│   │   └── convert_model_to_c.py      # Transpiles Python decision trees into C++ header code
│   ├── esp32_firmware/
│   │   └── fall_detection/
│   │       ├── fall_detection.ino     # ML-based ESP32 firmware with local web dashboard
│   │       └── rf_model.h             # Embedded RF classifier decision trees
│   ├── check_labels .py               # Utility to check class distributions in features
│   └── test_model .py                 # Python script to run local tests on features.csv
```

---

<b>Firmware & Code Analysis</b>

This project runs an advanced machine learning-based fall detection firmware on the ESP32 which combines threshold triggers with a <b>Random Forest Classifier</b>.

<b>ESP32 Firmware (fall_detection_rf/esp32_firmware/fall_detection/)</b>

<b>Hybrid Sensing:</b>
  <b>Uses a circular buffer of size 200 (WINDOW_SIZE) storing 3-axis acceleration and 3-axis gyroscope data at ~100Hz (delay(10) loop).</b>
  <b>Accelerometer range set to \pm 8g (MPU6050_ACCEL_FS_8) and normalized using / 4096.0.</b>
<b>Trigger Mechanism:</b>
  <b>Uses physics heuristics to trigger the classifier: If TotalAccel goes below 0.5g (free fall) and then exceeds 3.0g (impact), it triggers.</b>
  <b>3 seconds after the impact trigger, it freezes the 200-sample window and processes it.</b>
<b>Feature Extraction on Edge:</b>
  <b>Computes 4 statistical metrics (Mean, Standard Deviation, Max, Min) over the 200-sample window for each of the 6 data axes (a_x, a_y, a_z, g_x, g_y, g_z).</b>
  <b>This generates a 24-dimensional feature vector: [ax_mean, ax_std, ax_max, ax_min, ..., gz_mean, gz_std, gz_max, gz_min]</b>
<b>Prediction:</b>
  <b>Passes the 24 features into rf_predict(features) from rf_model.h.</b>
  <b>rf_model.h contains 20 decision trees transpiled into C/C++ nested if-else conditionals.</b>
  <b>The model outputs a class prediction (1 for Fall, 0 for Normal Activity) via majority voting.</b>
<b>Web UI Server:</b>
  <b>Runs a background web server (WebServer server(80)) on the ESP32.</b>
  <b>Serves a live-updating dashboard showing the device's status and last known/live GPS coordinates.</b>
<b>Secure Communication:</b>
  <b>Uses WiFiClientSecure with client.setInsecure() to securely handle SSL handshake with the Twilio API over HTTPS (port 443).</b>

---

<b>Machine Learning Training Pipeline</b>

If you want to retrain the Random Forest model, the Python scripts in `fall_detection_rf/src/` outline the full training pipeline:

1. **`prepare_dataset.py`**: Reads raw txt data from the **SisFall** dataset. Files starting with `D` indicate daily activities (Label `0`), and files starting with `F` indicate falls (Label `1`). It outputs a merged dataset: `labeled_sensor_data.csv`.
2. **`feature_extraction.py`**: Scales raw values ($\div 16384$ for accel and $\div 131$ for gyro) to match the MPU6050 physical unit representation. Extracts statistical windows of size 200 (mean, std, max, min) and outputs `features.csv`.
3. **`train_rf_model.py`**: Loads `features.csv`, performs an 80/20 train/test split, trains a Random Forest model (20 trees, max depth 8), and saves the model as a serialized joblib file (`random_forest_model.pkl`).
4. **`convert_model_to_c.py`**: Automatically traverses the structures of the trained estimators inside the scikit-learn model, outputs them as C-nested decisions in `rf_model.h`, and appends the majority voting algorithm.

---

<b>Setup & Upload Instructions</b>

Follow these steps to upload the firmware to your ESP32 using the Arduino IDE:

<b>1. Arduino IDE Setup</b>
1. Download and install <b>Arduino IDE</b> (version 2.x recommended).
2. Go to **File > Preferences**.
3. Under <b>Additional Boards Manager URLs</b>, paste:
   `https://raw.githubusercontent.com/espressif/arduino-esp32/gh-pages/package_esp32_index.json`
4. Go to **Tools > Board > Boards Manager...**, search for **esp32** by Espressif Systems, and click **Install**.
5. Go to **Tools > Library Manager...** and install these libraries:
   <b>MPU6050 by Jeff Rowberg (or Electronic Cats)</b>
   <b>TinyGPS++ by Mikal Hart</b>
   <b>I2Cdev (required dependency for MPU6050)</b>

<b>2. Wiring Connections</b>

| ESP32 Pin | MPU6050 Pin | Neo-6M GPS Pin | Buzzer / LED Pin |
| :--- | :--- | :--- | :--- |
| <b>3V3</b> / <b>VIN</b> | VCC | VCC | VCC / Anode (+) |
| <b>GND</b> | GND | GND | GND / Cathode (-) |
| <b>GPIO 21</b> | SDA | - | - |
| <b>GPIO 22</b> | SCL | - | - |
| <b>GPIO 16 (RX2)</b> | - | TX | - |
| <b>GPIO 17 (TX2)</b> | - | RX | - |
| <b>GPIO 15</b> | - | - | Buzzer / LED Input |

*Note: In the ML code, the GPS uses `HardwareSerial(1)` on pins 16 & 17, and the MPU6050 uses standard I2C on pins 21 & 22.*

<b>3. Twilio & WiFi Configuration</b>
In [fall_detection.ino](file:///c:/miniproject/fall_detection_rf/esp32_firmware/fall_detection/fall_detection.ino), configure your local WiFi network details and your Twilio developer API tokens:

```cpp
// WiFi Configuration
const char* ssid = "YOUR_WIFI_NAME";
const char* password = "YOUR_WIFI_PASSWORD";

// Twilio Configuration
String accountSID = "YOUR_TWILIO_ACCOUNT_SID";
String authToken  = "YOUR_TWILIO_AUTH_TOKEN";
String fromNumber = "YOUR_TWILIO_PHONE_NUMBER"; // e.g. +16616055991
String toNumber   = "CAREGIVER_PHONE_NUMBER";   // e.g. +91XXXXXXXXXX
```

<b>4. Uploading to ESP32</b>
1. Connect the ESP32 to your PC using a micro-USB (data) cable.
2. Under **Tools > Board**, select **ESP32 Dev Module** (or your specific ESP32 variant).
3. Under **Tools > Port**, select the COM Port corresponding to your connected ESP32.
4. Click the <b>Verify</b> (checkmark) button to compile the code.
5. Click the <b>Upload</b> (arrow) button to upload the binary.
   *(Note: If the upload hangs at "Connecting...", press and hold the <b>BOOT</b> button on the ESP32 until the flashing progress starts).*
6. Open **Tools > Serial Monitor** and set the baud rate to `115200` to view real-time debug outputs.
