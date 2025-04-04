# Sound locator filter

![image](https://github.com/user-attachments/assets/7a0e1f4a-c19f-4895-93ae-e880d86f8ce6)

The goal of this project is to design and implement a power-efficient, multi-sensor fusion system on ESP32 that combines radar and audio data for enhanced environmental monitoring. The system integrates 4G/5G wireless communication and focuses on pre-processing of sensors' data.

Multi-Sensor Fusion
- Integration of radar sensors (e.g., Infineon BGT60LTR11AIP, Texas Instruments IWR6843) for robust object detection and tracking.
- Incorporation of MEMS microphones (e.g., INMP441, SPH0645LM4H) for high-sensitivity audio capture.
- Use of environmental sensors (e.g., Bosch BME280) for monitoring temperature, humidity, and pressure to provide contextual environmental data.

Wireless Communication
- Interface Quectel EC25/EG25-G (4G LTE) for 4G/5G connectivity for remote data transmission and real-time monitoring.

Sensors data processing
- Advanced noise cancellation and sound synthesis for improved audio analysis and detection.
- ESP32: Collects sensor data, applies a Kalman filter, and forwards preprocessed data. (No image classification, heat maps or heavy signal processing on ESP32).
- Incorporate Kalman filter algorithms for precise tracking, noise reduction, and accurate estimation of object states using radar and audio data.

Actuators
- Speakers (e.g., Miniature Mylar Speakers) for sound synthesis and audio playback.
- Vibratory actuators (e.g., LRA Motors) for haptic feedback and environmental alerts.

Power Efficiency
- Optimization of firmware for low-power consumption.
