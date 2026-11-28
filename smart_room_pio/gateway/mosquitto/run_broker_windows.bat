@echo off
REM Cách 2 (Windows, đã cài Mosquitto): bấm đúp file này. Cửa sổ hiện log kết nối của board.
net stop mosquitto >nul 2>&1
"C:\Program Files\mosquitto\mosquitto.exe" -c "%~dp0mosquitto.conf" -v
pause
