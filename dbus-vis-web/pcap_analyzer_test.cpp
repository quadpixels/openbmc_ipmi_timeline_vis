#include "gtest/gtest.h"

#include "pcap_analyzer.hpp"

TEST(PcapAnalyzerTest, UTF8EncodingTest) {
    std::string utf8string = "哈"; // code point 21704, {0xe5, 0x93, 0x88} in UTF-8, {0xb9, 0xfe} in GBK
    ASSERT_EQ(utf8string.size(), 3);
}

TEST(PcapAnalyzerTest, DecodeUint32Test) {
    std::vector<uint8_t> bytes = { 1, 2, 3, 4 };
    AlignedStream stream;
    stream.buf = &bytes;
    TypeContainer tc;
    tc.type = DBusDataType::UINT32;
    ASSERT_EQ(std::get<uint32_t>(ParseFixed(MessageEndian::LITTLE,
        &stream, tc)), 0x04030201);
}

TEST(PcapAnalyzerTest, ParseMessageHeaderTest) {
    std::vector<uint8_t> bytes = {
        0x00, 0x00, 0x00, 0x00, 0x9d, 0x00, 0x00, 0x00,
        0x01, 0x01, 0x6f, 0x00, 0x2c, 0x00, 0x00, 0x00, 0x2f, 0x78, 0x79, 0x7a, 0x2f, 0x6f, 0x70, 0x65,
        0x6e, 0x62, 0x6d, 0x63, 0x5f, 0x70, 0x72, 0x6f, 0x6a, 0x65, 0x63, 0x74, 0x2f, 0x73, 0x65, 0x6e,
        0x73, 0x6f, 0x72, 0x73, 0x2f, 0x75, 0x74, 0x69, 0x6c, 0x69, 0x7a, 0x61, 0x74, 0x69, 0x6f, 0x6e,
        0x2f, 0x43, 0x50, 0x55, 0x00, 0x00, 0x00, 0x00, 0x02, 0x01, 0x73, 0x00, 0x1f, 0x00, 0x00, 0x00,
        0x6f, 0x72, 0x67, 0x2e, 0x66, 0x72, 0x65, 0x65, 0x64, 0x65, 0x73, 0x6b, 0x74, 0x6f, 0x70, 0x2e,
        0x44, 0x42, 0x75, 0x73, 0x2e, 0x50, 0x72, 0x6f, 0x70, 0x65, 0x72, 0x74, 0x69, 0x65, 0x73, 0x00,
        0x03, 0x01, 0x73, 0x00, 0x11, 0x00, 0x00, 0x00, 0x50, 0x72, 0x6f, 0x70, 0x65, 0x72, 0x74, 0x69,
        0x65, 0x73, 0x43, 0x68, 0x61, 0x6e, 0x67, 0x65, 0x64, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x08, 0x01, 0x67, 0x00, 0x08, 0x73, 0x61, 0x7b, 0x73, 0x76, 0x7d, 0x61, 0x73, 0x00, 0x00, 0x00,
        0x07, 0x01, 0x73, 0x00, 0x04, 0x00, 0x00, 0x00, 0x3a, 0x31, 0x2e, 0x37, 0x00, 0x00, 0x00, 0x00,
    };
    AlignedStream stream;
    stream.buf = &bytes;
    stream.offset = 4;
    TypeContainer tc = ParseOneSignature("a(yv)");
    // Should not crash
    DBusType val = ParseType(MessageEndian::LITTLE, &stream, tc).value();
    ASSERT_TRUE(std::holds_alternative<DBusContainer>(val));
    DBusContainer array = std::get<DBusContainer>(val);
    ASSERT_EQ(array.values.size(), 5); // o, s, s, g, s
    
    DBusVariant v;

    // Header Field Type=1 (PATH), value is an object_path
    DBusContainer struct0 = std::get<DBusContainer>(array.values[0]);
    v = std::get<DBusVariant>(struct0.values[0]);
    ASSERT_EQ(std::get<uint8_t>(v), 1);
    v = std::get<DBusVariant>(struct0.values[1]);
    ASSERT_EQ(std::get<object_path>(v).str, "/xyz/openbmc_project/sensors/utilization/CPU");

    // Type=2 (INTERFACE), value is a string
    DBusContainer struct1 = std::get<DBusContainer>(array.values[1]);
    v = std::get<DBusVariant>(struct1.values[0]);
    ASSERT_EQ(std::get<uint8_t>(v), 2);
    v = std::get<DBusVariant>(struct1.values[1]);
    ASSERT_EQ(std::get<std::string>(v), "org.freedesktop.DBus.Properties");

    // Type=3 (MEMBER), value is a string
    DBusContainer struct2 = std::get<DBusContainer>(array.values[2]);
    v = std::get<DBusVariant>(struct2.values[0]);
    ASSERT_EQ(std::get<uint8_t>(v), 3);
    v = std::get<DBusVariant>(struct2.values[1]);
    ASSERT_EQ(std::get<std::string>(v), "PropertiesChanged");

    // Type=8 (SIGNATURE), value is a signature
    DBusContainer struct3 = std::get<DBusContainer>(array.values[3]);
    v = std::get<DBusVariant>(struct3.values[0]);
    ASSERT_EQ(std::get<uint8_t>(v), 8);
    v = std::get<DBusVariant>(struct3.values[1]);
    ASSERT_EQ(std::get<std::string>(v), "sa{sv}as");

    // TYPE=7 (SENDER), value is a string
    DBusContainer struct4 = std::get<DBusContainer>(array.values[4]);
    v = std::get<DBusVariant>(struct4.values[0]);
    ASSERT_EQ(std::get<uint8_t>(v), 7);
    v = std::get<DBusVariant>(struct4.values[1]);
    ASSERT_EQ(std::get<std::string>(v), ":1.7");
}

// A Bluetooth packet that caused an error
TEST(PcapAnalyzerTest, ParsePacketTest) {
    std::vector<uint8_t> bytes = {
        0x6c, 0x02, 0x01, 0x01, 0x00, 0x00, 0x00, 0x00, 0x1b, 0x00, 0x00, 0x00, 0x2f, 0x00, 0x00, 0x00,
        0x06, 0x01, 0x73, 0x00, 0x06, 0x00, 0x00, 0x00, 0x3a, 0x31, 0x2e, 0x34, 0x35, 0x37, 0x00, 0x00,
        0x08, 0x01, 0x67, 0x00, 0x00, 0x00, 0x00, 0x00, 0x05, 0x01, 0x75, 0x00, 0x03, 0x00, 0x00, 0x00,
        0x07, 0x01, 0x73, 0x00, 0x06, 0x00, 0x00, 0x00, 0x3a, 0x31, 0x2e, 0x34, 0x35, 0x38, 0x00, 0x00,
    };

    MessageEndian endian;
    FixedHeader fixed;
    DBusMessageFields fields;
    std::vector<DBusType> body;
    ParseHeaderAndBody(bytes.data(), bytes.size(), &fixed, &fields, &body);
    fixed.Print();
}

TEST(PcapAnalzerTest, MultipleMembersInSignature) {
    std::vector<TypeContainer> tcs = ParseSignatures("sa{sv}as");
    ASSERT_EQ(tcs.size(), 3);
    ASSERT_EQ(tcs[0].ToString(), "s");
    ASSERT_EQ(tcs[1].ToString(), "a{sv}");
    ASSERT_EQ(tcs[2].ToString(), "as");
}

int main(int argc, char **argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}