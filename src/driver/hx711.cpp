#include "hx711.hpp"
#include <wiringPi.h>
#include <thread>
#include <chrono>
#include <stdexcept>
#include <vector>
#include <algorithm>
#include <numeric>
#include <iostream>

HX711::HX711(int dout, int pd_sck, int gain) {
    PD_SCK = pd_sck;
    DOUT = dout;

    pinMode(PD_SCK, OUTPUT);
    pinMode(DOUT, INPUT);

    set_gain(gain);

    std::this_thread::sleep_for(std::chrono::seconds(1));
}

long HX711::convertFromTwosComplement24bit(long inputValue) {
    return -(inputValue & 0x800000) + (inputValue & 0x7FFFFF);
}

bool HX711::is_ready() {
    return digitalRead(DOUT) == LOW;
}

void HX711::set_gain(int gain) {
    if (gain == 128) {
        GAIN = 1;
    } else if (gain == 64) {
        GAIN = 3;
    } else if (gain == 32) {
        GAIN = 2;
    }

    digitalWrite(PD_SCK, LOW);
    readRawBytes();
}

int HX711::get_gain() {
    if (GAIN == 1) return 128;
    if (GAIN == 3) return 64;
    if (GAIN == 2) return 32;
    return 0;
}

uint8_t HX711::readNextBit() {
    digitalWrite(PD_SCK, HIGH);
    digitalWrite(PD_SCK, LOW);
    return digitalRead(DOUT);
}

uint8_t HX711::readNextByte() {
    uint8_t byteValue = 0;
    for (int i = 0; i < 8; ++i) {
        if (bit_format == "MSB") {
            byteValue <<= 1;
            byteValue |= readNextBit();
        } else {
            byteValue >>= 1;
            byteValue |= (readNextBit() * 0x80);
        }
    }
    return byteValue;
}

std::vector<uint8_t> HX711::readRawBytes() {
    std::lock_guard<std::mutex> lock(readLock);

    auto startTime = std::chrono::steady_clock::now();
    auto timeoutDuration = std::chrono::milliseconds(2000);

    while (!is_ready()) {
        if (timeouts > maxTimeouts) {
            sensor_ok = false;
            return {0, 0, 0};
        }
        auto now = std::chrono::steady_clock::now();
        if (now - startTime > timeoutDuration) {
            timeouts++;
            std::cout << "Timeout reached for HX711 "; 
            std::cout << "DT: " << DOUT << " ";
            std::cout << "SCK: " << PD_SCK << std::endl;
            return {0, 0, 0};
        }
        // yield or sleep briefly
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
    timeouts = 0;

    uint8_t firstByte = readNextByte();
    uint8_t secondByte = readNextByte();
    uint8_t thirdByte = readNextByte();

    for (int i = 0; i < GAIN; ++i) {
        readNextBit(); // Clock out gain setting bits
    }

    if (byte_format == "LSB") {
        return {thirdByte, secondByte, firstByte};
    } else {
        return {firstByte, secondByte, thirdByte};
    }
}

long HX711::read_long() {
    std::vector<uint8_t> dataBytes = readRawBytes();

    if (DEBUG_PRINTING) {
        std::cout << "Raw Bytes: " << (int)dataBytes[0] << " " << (int)dataBytes[1] << " " << (int)dataBytes[2] << std::endl;
    }

    long twosComplementValue = (static_cast<long>(dataBytes[0]) << 16) |
                               (static_cast<long>(dataBytes[1]) << 8) |
                               dataBytes[2];

    if (DEBUG_PRINTING) {
        printf("Twos: 0x%06lx\n", twosComplementValue);
    }
    
    long signedIntValue = convertFromTwosComplement24bit(twosComplementValue);
    lastVal = signedIntValue;
    return signedIntValue;
}

long HX711::read_average(int times) {
    if (times <= 0) {
        throw std::invalid_argument("HX711::read_average(): times must be >= 1!");
    }
    if (times == 1) {
        return read_long();
    }
    if (times < 5) {
        return read_median(times);
    }

    std::vector<long> valueList;
    for (int i = 0; i < times; ++i) {
        valueList.push_back(read_long());
    }
    std::sort(valueList.begin(), valueList.end());

    int trimAmount = static_cast<int>(valueList.size() * 0.2);
    
    // Create a sub-vector to trim outliers
    std::vector<long> trimmedList(valueList.begin() + trimAmount, valueList.end() - trimAmount);

    long long sum = std::accumulate(trimmedList.begin(), trimmedList.end(), 0LL);
    return static_cast<long>(sum / trimmedList.size());
}

double HX711::read_median(int times) {
    if (times <= 0) {
        throw std::invalid_argument("HX711::read_median(): times must be > 0!");
    }
    if (times == 1) {
        return read_long();
    }

    std::vector<long> valueList;
    for (int i = 0; i < times; ++i) {
        valueList.push_back(read_long());
    }
    std::sort(valueList.begin(), valueList.end());

    if (times % 2 == 1) {
        return valueList[valueList.size() / 2];
    } else {
        size_t midpoint = valueList.size() / 2;
        return (valueList[midpoint - 1] + valueList[midpoint]) / 2.0;
    }
}

double HX711::get_value(int times) {
    return get_value_A(times);
}

double HX711::get_value_A(int times) {
    return read_median(times) - get_offset_A();
}

double HX711::get_value_B(int times) {
    int current_gain = get_gain();
    set_gain(32);
    double value = read_median(times) - get_offset_B();
    set_gain(current_gain);
    return value;
}

double HX711::get_weight(int times) {
    return get_weight_A(times);
}

double HX711::get_weight_A(int times) {
    double value = get_value_A(times);
    return value / REFERENCE_UNIT;
}

double HX711::get_weight_B(int times) {
    double value = get_value_B(times);
    return value / REFERENCE_UNIT_B;
}

void HX711::tare(int times) {
    tare_A(times);
}

void HX711::tare_A(int times) {
    double backupRefUnit = get_reference_unit_A();
    set_reference_unit_A(1);

    long value = read_average(times);
    if (DEBUG_PRINTING) {
        std::cout << "Tare A value: " << value << std::endl;
    }
    set_offset_A(value);

    set_reference_unit_A(backupRefUnit);
}

void HX711::tare_B(int times) {
    double backupRefUnit = get_reference_unit_B();
    set_reference_unit_B(1);

    int backupGain = get_gain();
    set_gain(32);
    
    long value = read_average(times);
    if (DEBUG_PRINTING) {
        std::cout << "Tare B value: " << value << std::endl;
    }
    set_offset_B(value);
    
    set_gain(backupGain);
    set_reference_unit_B(backupRefUnit);
}

void HX711::set_reading_format(const std::string& b_format, const std::string& bi_format) 
{
    if (b_format == "LSB" || b_format == "MSB") {
        byte_format = b_format;
    } else {
        throw std::invalid_argument("Unrecognized byte_format");
    }
    if (bi_format == "LSB" || bi_format == "MSB") {
        bit_format = bi_format;
    } else {
        throw std::invalid_argument("Unrecognized bit_format");
    }
}

void HX711::set_offset(long offset) { set_offset_A(offset); }
void HX711::set_offset_A(long offset) { OFFSET = offset; }
void HX711::set_offset_B(long offset) { OFFSET_B = offset; }
long HX711::get_offset() { return get_offset_A(); }
long HX711::get_offset_A() { return OFFSET; }
long HX711::get_offset_B() { return OFFSET_B; }

void HX711::set_reference_unit(double ref_unit) { set_reference_unit_A(ref_unit); }
void HX711::set_reference_unit_A(double ref_unit) {
    if (ref_unit == 0) throw std::invalid_argument("Reference unit cannot be 0");
    REFERENCE_UNIT = ref_unit;
}
void HX711::set_reference_unit_B(double ref_unit) {
    if (ref_unit == 0) throw std::invalid_argument("Reference unit cannot be 0");
    REFERENCE_UNIT_B = ref_unit;
}
double HX711::get_reference_unit() { return get_reference_unit_A(); }
double HX711::get_reference_unit_A() { return REFERENCE_UNIT; }
double HX711::get_reference_unit_B() { return REFERENCE_UNIT_B; }


void HX711::power_down() {
    std::lock_guard<std::mutex> lock(readLock);
    digitalWrite(PD_SCK, LOW);
    digitalWrite(PD_SCK, HIGH);
    std::this_thread::sleep_for(std::chrono::microseconds(100));
}

void HX711::power_up() {
    std::lock_guard<std::mutex> lock(readLock);
    digitalWrite(PD_SCK, LOW);
    std::this_thread::sleep_for(std::chrono::microseconds(100));
    
    if (get_gain() != 128) {
        readRawBytes();
    }
}

void HX711::reset() {
    power_down();
    power_up();
}
