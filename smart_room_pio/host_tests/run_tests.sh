#!/bin/sh
# Chạy test phần lõi trên máy tính (không cần board). Chạy: sh host_tests/run_tests.sh
cd "$(dirname "$0")" || exit 1
L=../lib/node_a_core
set -e
gcc -Wall -Wextra -I$L test_sensor_a.c $L/sensor_manager.c $L/input_filter.c $L/sensor_mock.c -o t1.out && ./t1.out | tail -1
gcc -Wall -Wextra -I$L test_modbus.c $L/modbus_rtu.c $L/sht20_modbus.c -o t2.out && ./t2.out | tail -1
gcc -Wall -Wextra -I$L test_net_msg.c $L/net_msg.c $L/cJSON.c -lm -o t3.out && ./t3.out | tail -1
gcc -Wall -Wextra -I$L test_dht20.c $L/dht20.c -o t4.out && ./t4.out | tail -1
rm -f t1.out t2.out t3.out t4.out
