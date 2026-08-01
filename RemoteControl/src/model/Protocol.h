#ifndef PROTOCOL_H
#define PROTOCOL_H

#include <QtGlobal>
#include <QByteArray>
#include <QString>

// ── Magic numbers for TCP binary framing ──
constexpr quint32 MAGIC_JSON  = 0x4A534F4E;  // "JSON"
constexpr quint32 MAGIC_FRAME = 0x46524D45;  // "FRME"
constexpr quint32 MAGIC_FILE  = 0x46494C45;  // "FILE"

// ── Default network settings ──
constexpr quint16 DEFAULT_HOST_PORT = 19527;

// ── Screen capture defaults ──
constexpr int DEFAULT_QUALITY = 35;   // JPEG quality 10-100（35 画质/速度最佳平衡）
constexpr int DEFAULT_FPS     = 10;   // frames per second

// ── Data model structs ──
struct FrameInfo {
    int width = 0;
    int height = 0;
    QByteArray jpegData;
};

struct DeviceInfo {
    QString deviceId;
    QString deviceName;
};

#endif // PROTOCOL_H
