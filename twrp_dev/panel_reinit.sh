#!/system/bin/sh
# Panel reinit script for Nokia N1 after kexec
# Root cause: GPIO-190 (disp0_rst) LOW = panel held in reset
# GPIO-189 (disp_bias_en) was already set HIGH in previous test

BAR0=0xC0000000
LP_GEN_CTRL=$((BAR0 + 0xB06C))
GEN_FIFO_STAT=$((BAR0 + 0xB074))

echo "=== Panel Reinit Script ==="
echo "Step 0: Check current state"
echo "GPIO-189 (bias): $(cat /sys/class/gpio/gpio189/direction)/$(cat /sys/class/gpio/gpio189/value)"

# Export GPIO 190 if needed
echo 190 > /sys/class/gpio/export 2>/dev/null
echo "GPIO-190 (rst):  $(cat /sys/class/gpio/gpio190/direction)/$(cat /sys/class/gpio/gpio190/value)"

echo ""
echo "Step 1: GPIO reset sequence"
# Ensure bias is high (power on)
echo out > /sys/class/gpio/gpio189/direction
echo 1 > /sys/class/gpio/gpio189/value
echo "  bias=HIGH"

# Reset pulse: ensure LOW first  
echo out > /sys/class/gpio/gpio190/direction
echo 0 > /sys/class/gpio/gpio190/value
echo "  rst=LOW (assert reset)"
usleep 5000  # 5ms

# Deassert reset
echo 1 > /sys/class/gpio/gpio190/value
echo "  rst=HIGH (deassert reset)"
usleep 10000  # 10ms for panel IC to initialize

echo ""
echo "Step 2: Check FIFO status"
FIFO=$(devmem $GEN_FIFO_STAT 32)
echo "  FIFO stat: $FIFO"

echo ""
echo "Step 3: Send DSI init commands (LP mode)"

# EXIT_SLEEP_MODE (0x11) - DCS Short Write 0 param (type 0x05)
echo "  Sending EXIT_SLEEP (0x11)..."
devmem $LP_GEN_CTRL 32 0x00001105
sleep 1  # Panel needs 100ms+ to exit sleep

# MCS Access Enable: Generic Short Write 2p (type 0x23)
# cmd=0xB0, param=0x04 → (0x04 << 16) | (0xB0 << 8) | 0x23
echo "  Sending MCS unlock (0xB0, 0x04)..."
devmem $LP_GEN_CTRL 32 0x0004B023

usleep 5000

# jdi_set_mode: Generic Long Write (type 0x29)  
# Data: 0xB3, 0x35 (2 bytes) → write to LP_GEN_DATA first, then ctrl
# LP_GEN_DATA = BAR0 + 0xB064
echo "  Sending jdi_set_mode long write..."
devmem $((BAR0 + 0xB064)) 32 0x000035B3
# Control: word_count=2, type=0x29 → (2 << 8) | 0x29 = 0x0229  
devmem $LP_GEN_CTRL 32 0x00000229

usleep 5000

# MCS Lock: Generic Short Write 2p
# cmd=0xB0, param=0x03
echo "  Sending MCS lock (0xB0, 0x03)..."
devmem $LP_GEN_CTRL 32 0x0003B023

usleep 5000

# SET_DISPLAY_ON (0x29) - DCS Short Write 0 param
echo "  Sending DISPLAY_ON (0x29)..."
devmem $LP_GEN_CTRL 32 0x00002905
usleep 30000  # 20ms+

# WRITE_CONTROL_DISPLAY (0x53) with param 0x24
# DCS Short Write 1p (type 0x15): (param << 16) | (cmd << 8) | 0x15
echo "  Sending WRITE_CTRL_DISPLAY (0x53, 0x24)..."
devmem $LP_GEN_CTRL 32 0x00245315

usleep 5000

# SET_DISPLAY_BRIGHTNESS (0x51) to max (0xFF)
echo "  Sending SET_BRIGHTNESS (0x51, 0xFF)..."
devmem $LP_GEN_CTRL 32 0x00FF5115

echo ""
echo "Step 4: Verify"
FIFO=$(devmem $GEN_FIFO_STAT 32)
echo "  FIFO stat: $FIFO"
echo "  Brightness: $(cat /sys/class/backlight/psb-bl/brightness)"
echo "  Actual:     $(cat /sys/class/backlight/psb-bl/actual_brightness)"
echo "  Regulator:  $(cat /sys/class/regulator/regulator.4/state)"

echo ""
echo "Done! Check if screen shows anything."
