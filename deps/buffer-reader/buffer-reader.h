#include <cstddef>
#include <cstdint>

namespace Buffer {

struct BufferResult {
    char* buffer;
    uint32_t length;
};

struct BufferRange {
    size_t start;
    size_t end;
};

class BufferReader {
public:
    BufferReader();
    BufferReader(char* buffer, size_t length);

    size_t offset;

    bool Error();

    uint8_t ReadUint8();
    uint16_t ReadUint16BE();
    uint32_t ReadUint32BE();
    void Skip(size_t bytes);
    void ReadUint8Buffer(struct BufferResult &result);
    void ReadUint16BERange(struct BufferRange &range);

    void WriteUint32BE(uint32_t value);

private:
    bool CheckRead(size_t length);
    bool error;
    char* buffer;
    size_t length;
};

}
