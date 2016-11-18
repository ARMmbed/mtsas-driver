#include "MTSText.h"
#include "ctype.h"
#include <math.h>

using namespace mts;

std::string BASE64CODE = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/=";

std::string Text::getLine(const std::string& source, const size_t& start, size_t& cursor) {
    char delimiters[2];
    delimiters[0] = '\n';
    delimiters[1] = '\r';
    size_t end = source.find_first_of(delimiters, start, 2);
    std::string line(source.substr(start, end - start));
    if (end < source.size()) {
        if (end < source.size() - 1)
            if ((source[end] == '\n' && source[end + 1] == '\r') || (source[end] == '\r' && source[end + 1] == '\n')) {
                //Advance an additional character in scenarios where lines end in \r\n or \n\r
                end++;
            }
        end++;
    }
    cursor = end;
    return line;
}

std::vector<std::string> Text::split(const std::string& str, char delimiter, int limit) {
    return split(str, std::string(1, delimiter), limit);
}

std::vector<std::string> Text::split(const std::string& str, const std::string& delimiter, int limit) {
    std::vector<std::string> result;
    if(str.size() == 0) {
        return result;
    }
    size_t start = 0;
    size_t end = str.find(delimiter, start);
    for (int i = 1; i < limit || (limit <= 0 && (end != std::string::npos)); ++i) {
        result.push_back(str.substr(start, end - start));
        start = end + delimiter.length();
        end = str.find(delimiter, start);
    }
    result.push_back(str.substr(start));
    return result;
}

std::string Text::readString(char* index, int length)
{
    std::string result = std::string(index, length);
    index += length;
    return result;
}

std::string Text::toUpper(const std::string str)
{
    std::string ret = str;

    for (unsigned int i = 0; i < ret.size(); i++)
    {
        ret[i] = toupper(ret[i]);
    }

    return ret;
}

std::string Text::float2String(double val, int precision) {
    char buff[100];
    sprintf(buff, "%d.%d", (int)val, (int)((val - floor(val)) * (int)pow(10.0, precision)));
    return std::string(buff);
}

std::string Text::bin2hexString(const std::vector<uint8_t>& data, const char* delim, bool leadingZeros) {
    uint8_t* data_arr = new uint8_t[data.size()];

    for (int i = 0; i < data.size(); i++)
        data_arr[i] = data[i];

    std::string s = bin2hexString(data_arr, data.size(), delim, leadingZeros);
    delete[] data_arr;
    return s;
}

std::string Text::bin2hexString(const uint8_t* data, const uint32_t len, const char* delim, bool leadingZeros) {
    std::string str;
    char buf[32];
    char lead[] = "0x";

    for (uint32_t i = 0; i < len; i++) {
        if (leadingZeros)
            str.append(lead);
        snprintf(buf, sizeof(buf), "%02x", data[i]);
        str.append(buf, strlen(buf));
        if (i < len - 1)
            str.append(delim);
    }

    return str;
}

std::string Text::bin2base64(const uint8_t* data, size_t size) {
    std::string out;
    uint8_t b;

    for (size_t i = 0; i < size; i+=3) {
        b = (data[i] & 0xfc) >> 2;
        out.push_back(BASE64CODE[b]);
        b = (data[i] & 0x03) << 4;
        if (i+1 < size) {
            b |= (data[i+1] & 0xf0) >> 4;
            out.push_back(BASE64CODE[b]);
            b = (data[i + 1] & 0x0f) << 2;
            if (i+2 < size) {
                b |= (data[i+2] & 0xc0) >> 6;
                out.push_back(BASE64CODE[b]);
                b = data[i+2] & 0x3f;
                out.push_back(BASE64CODE[b]);
            } else {
                out.push_back(BASE64CODE[b]);
                out.append("=");
            }
        } else {
            out.push_back(BASE64CODE[b]);
            out.append("==");
        }
    }

    return out;
}

std::string Text::bin2base64(const std::vector<uint8_t>& data) {
    std::string out;
    uint8_t b;

    for (size_t i = 0; i < data.size(); i+=3) {
        b = (data[i] & 0xfc) >> 2;
        out.push_back(BASE64CODE[b]);
        b = (data[i] & 0x03) << 4;
        if (i+1 < data.size()) {
            b |= (data[i+1] & 0xf0) >> 4;
            out.push_back(BASE64CODE[b]);
            b = (data[i + 1] & 0x0f) << 2;
            if (i+2 < data.size()) {
                b |= (data[i+2] & 0xc0) >> 6;
                out.push_back(BASE64CODE[b]);
                b = data[i+2] & 0x3f;
                out.push_back(BASE64CODE[b]);
            } else {
                out.push_back(BASE64CODE[b]);
                out.append("=");
            }
        } else {
            out.push_back(BASE64CODE[b]);
            out.append("==");
        }
    }

    return out;
}

bool Text::base642bin(const std::string in, std::vector<uint8_t>& out) {

    if (in.find_first_not_of(BASE64CODE) != std::string::npos) {
        return false;
    }

    if (in.size() % 4 != 0) {
        return false;
    }

    uint8_t a,b,c,d;

    for (uint32_t i = 0; i < in.size(); i+=4) {
        a = BASE64CODE.find(in[i]);
        b = BASE64CODE.find(in[i+1]);
        c = BASE64CODE.find(in[i+2]);
        d = BASE64CODE.find(in[i+3]);
        out.push_back(a << 2 | b >> 4);
        out.push_back(b << 4 | c >> 2);
        out.push_back(c << 6 | d);
    }

    return true;
}


void Text::ltrim(std::string& str, const char* args) {
    size_t startpos = str.find_first_not_of(args);
    if (startpos != std::string::npos)
        str = str.substr(startpos);
}

void Text::rtrim(std::string& str, const char* args) {
    size_t endpos = str.find_last_not_of(args);
    if (endpos != std::string::npos)
        str = str.substr(0, endpos + 1);
}

void Text::trim(std::string& str, const char* args) {
    ltrim(str, args);
    rtrim(str, args);
}
