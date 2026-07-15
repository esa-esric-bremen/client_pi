#pragma once
// based on hx711py

#include <cstdint>
#include <string>
#include <mutex>
#include <vector>

class HX711 {
public:
    HX711(int dout, int pd_sck, int gain = 128);

    // Check if the HX711 is ready.
    bool is_ready();

    // Set the gain factor; takes effect only after a call to read().
    void set_gain(int gain = 128);
    int get_gain();

    // Read a raw value.
    long read_long();
    
    // Read an average value.
    long read_average(int times = 3);
    
    // Read a median value.
    double read_median(int times = 3);

    // Get value (A) with offset removed.
    double get_value(int times = 3);
    double get_value_A(int times = 3);
    
    // Get value (B) with offset removed.
    double get_value_B(int times = 3);

    // Get weight (A).
    double get_weight(int times = 3);
    double get_weight_A(int times = 3);
    
    // Get weight (B).
    double get_weight_B(int times = 3);

    // Tare (A): set the current reading as the zero point.
    void tare(int times = 15);
    void tare_A(int times = 15);
    
    // Tare (B).
    void tare_B(int times = 15);

    // Set/get reading format.
    void set_reading_format(const std::string& byte_format = "MSB", const std::string& bit_format = "MSB");

    // Set/get offset (A).
    void set_offset(long offset);
    void set_offset_A(long offset);
    long get_offset();
    long get_offset_A();
    
    // Set/get offset (B).
    void set_offset_B(long offset);
    long get_offset_B();

    // Set/get reference unit (A).
    void set_reference_unit(double ref_unit);
    void set_reference_unit_A(double ref_unit);
    double get_reference_unit();
    double get_reference_unit_A();

    // Set/get reference unit (B).
    void set_reference_unit_B(double ref_unit);
    double get_reference_unit_B();

    // Power down and power up the HX711.
    void power_down();
    void power_up();

    // Reset the HX711.
    void reset();

    // For debugging
    bool DEBUG_PRINTING = false;

private:
    bool sensor_ok = true;
    // max. number of timeouts in a row we call the sensor not-okay
    int maxTimeouts = 5;
    int timeouts = 0;

    int PD_SCK;
    int DOUT;
    int GAIN;
    long OFFSET = 0;
    long OFFSET_B = 0;
    double REFERENCE_UNIT = 1;
    double REFERENCE_UNIT_B = 1;
    long lastVal = 0;
    std::string byte_format = "MSB";
    std::string bit_format = "MSB";
    std::mutex readLock;

    long convertFromTwosComplement24bit(long inputValue);
    uint8_t readNextBit();
    uint8_t readNextByte();
    std::vector<uint8_t> readRawBytes();
};
