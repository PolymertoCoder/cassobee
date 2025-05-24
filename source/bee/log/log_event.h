#pragma once
#include "rpcdata.h"
#include <string>

/* generate by progen.py
<rpcdata name="log_event" codefield="short">
    <field name="process_name" type="std::string" default="std::string()"/>
    <field name="filename" type="std::string" default="std::string()"/>
    <field name="line" type="uint16_t" default="0"/>
    <field name="timestamp" type="uint64_t" default="0"/>
    <field name="threadid" type="uint32_t" default="0"/>
    <field name="fiberid" type="uint32_t" default="0"/>
    <field name="elapse" type="std::string" default="std::string()"/>
    <field name="content" type="std::string" default="std::string()"/>
</rpcdata>
*/

namespace bee
{

class log_event : public rpcdata
{
public:
    enum FIELDS
    {
        FIELDS_PROCESS_NAME = 1 << 0,
        FIELDS_FILENAME = 1 << 1,
        FIELDS_LINE = 1 << 2,
        FIELDS_TIMESTAMP = 1 << 3,
        FIELDS_THREADID = 1 << 4,
        FIELDS_FIBERID = 1 << 5,
        FIELDS_ELAPSE = 1 << 6,
        FIELDS_CONTENT = 1 << 7,
        ALLFIELDS = (1 << 8) - 1
    };

    log_event() = default;
    log_event(const std::string& _process_name, const std::string& _filename, uint16_t _line, uint64_t _timestamp, uint32_t _threadid, uint32_t _fiberid, const std::string& _elapse, const std::string& _content) : code(0), process_name(_process_name), filename(_filename), line(_line), timestamp(_timestamp), threadid(_threadid), fiberid(_fiberid), elapse(_elapse), content(_content)
    {}
    log_event(std::string&& _process_name, std::string&& _filename, uint16_t _line, uint64_t _timestamp, uint32_t _threadid, uint32_t _fiberid, std::string&& _elapse, std::string&& _content) : code(0), process_name(std::move(_process_name)), filename(std::move(_filename)), line(_line), timestamp(_timestamp), threadid(_threadid), fiberid(_fiberid), elapse(std::move(_elapse)), content(std::move(_content))
    {}
    log_event(const log_event& rhs) = default;
    log_event(log_event&& rhs) = default;

    log_event& operator=(const log_event& rhs) = default;
    bool operator==(const log_event& rhs) const
    {
        return code == rhs.code && process_name == rhs.process_name && filename == rhs.filename && line == rhs.line && timestamp == rhs.timestamp && threadid == rhs.threadid && fiberid == rhs.fiberid && elapse == rhs.elapse && content == rhs.content;
    }
    bool operator!=(const log_event& rhs) const { return !(*this == rhs); }
    virtual ~log_event() override = default;
    octetsstream& pack(octetsstream& os) const override;
    octetsstream& unpack(octetsstream& os) override;

    virtual rpcdata* dup() const override { return new log_event(*this); };
    virtual ostringstream& dump(ostringstream& out) const override;

public:
    void set_process_name()
    {
        code |= FIELDS_PROCESS_NAME;
    }
    void set_process_name(const std::string& value)
    {
        code |= FIELDS_PROCESS_NAME;
        process_name = value;
    }
    void set_filename()
    {
        code |= FIELDS_FILENAME;
    }
    void set_filename(const std::string& value)
    {
        code |= FIELDS_FILENAME;
        filename = value;
    }
    void set_line()
    {
        code |= FIELDS_LINE;
    }
    void set_line(const uint16_t& value)
    {
        code |= FIELDS_LINE;
        line = value;
    }
    void set_timestamp()
    {
        code |= FIELDS_TIMESTAMP;
    }
    void set_timestamp(const uint64_t& value)
    {
        code |= FIELDS_TIMESTAMP;
        timestamp = value;
    }
    void set_threadid()
    {
        code |= FIELDS_THREADID;
    }
    void set_threadid(const uint32_t& value)
    {
        code |= FIELDS_THREADID;
        threadid = value;
    }
    void set_fiberid()
    {
        code |= FIELDS_FIBERID;
    }
    void set_fiberid(const uint32_t& value)
    {
        code |= FIELDS_FIBERID;
        fiberid = value;
    }
    void set_elapse()
    {
        code |= FIELDS_ELAPSE;
    }
    void set_elapse(const std::string& value)
    {
        code |= FIELDS_ELAPSE;
        elapse = value;
    }
    void set_content()
    {
        code |= FIELDS_CONTENT;
    }
    void set_content(const std::string& value)
    {
        code |= FIELDS_CONTENT;
        content = value;
    }
    void set_all_fields() { code = ALLFIELDS; }
    void clean_default()
    {
        if (process_name == std::string()) code &= ~FIELDS_PROCESS_NAME;
        if (filename == std::string()) code &= ~FIELDS_FILENAME;
        if (line == 0) code &= ~FIELDS_LINE;
        if (timestamp == 0) code &= ~FIELDS_TIMESTAMP;
        if (threadid == 0) code &= ~FIELDS_THREADID;
        if (fiberid == 0) code &= ~FIELDS_FIBERID;
        if (elapse == std::string()) code &= ~FIELDS_ELAPSE;
        if (content == std::string()) code &= ~FIELDS_CONTENT;
    }

public:
    short code = 0;
    std::string process_name = std::string();
    std::string filename = std::string();
    uint16_t line = 0;
    uint64_t timestamp = 0;
    uint32_t threadid = 0;
    uint32_t fiberid = 0;
    std::string elapse = std::string();
    std::string content = std::string();
};

} // namespace bee