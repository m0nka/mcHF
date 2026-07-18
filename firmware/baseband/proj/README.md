Introduction 

This directory contains the STM32CubeIDE project (mchf_baseband) for the CM4 baseband core,
based on the UHSDR firmware (sources in firmware/baseband)

Import into STM32CubeIDE

- In the File menu, select 'Open Projects from File System...'
- Select root directory: "firmware\baseband\proj", 'Detect and configure project natures' selected
- Click Finish

Note: SystemWorks no longer supported!

Troubleshooting

- Fix for functions with wrong inline definition - use optimization (O1 at least), more info:
	https://stackoverflow.com/questions/41218006/gcc-fails-to-inline-functions-without-o2

For any problems compiling under windows please open an issue report on the project GitHub page.

Thank you!

Krassi Atanassov, M0NKA

