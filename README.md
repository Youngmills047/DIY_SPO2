# DIY_SPO2
DIY SpO₂ and Heart Rate Monitor

This project is a DIY pulse oximeter built around the MAX30102 sensor, Arduino Nano, and an SSD1306 OLED display.
It measures blood oxygen saturation (SpO₂) and heart rate (BPM) in real-time.

The repository includes:

Firmware (Arduino code)

Hardware (KiCad schematic & PCB)

Documentation (wiring diagrams, PCB images, BOM, etc.)

🚀 Features

Reads SpO₂ (%) and heart rate (BPM) using the MAX30102 sensor

Real-time display on 0.96" SSD1306 OLED screen

Compact PCB design (KiCad project + Gerbers included)

Open-source and customizable

🛠 Components Used

Arduino Nano (ATmega328P)

MAX30102 Pulse Oximeter & Heart Rate Sensor

SSD1306 OLED Display (128x64, I2C)

Misc: Resistors, capacitors, push buttons, connectors

🔧 Getting Started

1️⃣ Clone the Repository

https://github.com/Youngmills047/DIY_SPO2.git

2️⃣ Install Arduino Libraries

Adafruit SSD1306

Adafruit GFX

MAX3010x (SparkFun library)

You can install them via Arduino IDE → Tools → Manage Libraries.

3️⃣ Upload the Firmware

Open spo2.ino in Arduino IDE

Select Arduino Nano and correct COM port

Upload the code

4️⃣ Hardware Setup

Connect MAX30102 → I2C pins (A4 = SDA, A5 = SCL)

Connect SSD1306 OLED → I2C pins (same bus)

Power from USB or regulated 5V

📐 Hardware (KiCad)

The full PCB design is available in /hardware/.

Open .kicad_pro in KiCad

Gerber files are in /exports/gerbers/ (ready for fabrication)

BOM file is included

⚠️ Disclaimer

This project is for educational purposes only.
It is not a medical-grade device and should not be used for diagnosis or treatment.
