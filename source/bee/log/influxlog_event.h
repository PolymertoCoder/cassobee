#pragma once
#include <map>
#include <string>
#include "rpcdata.h"

namespace bee
{

class influxlog_event : public rpcdata
{
public:
    influxlog_event() = default;
    influxlog_event(const std::string& _measurement, const std::map<std::string, std::string>& _tags, const std::map<std::string, std::string>& _fields, int64_t _timestamp) : measurement(_measurement), tags(_tags), fields(_fields), timestamp(_timestamp)
    {}
    influxlog_event(std::string&& _measurement, std::map<std::string, std::string>&& _tags, std::map<std::string, std::string>&& _fields, int64_t _timestamp) : measurement(std::move(_measurement)), tags(std::move(_tags)), fields(std::move(_fields)), timestamp(_timestamp)
    {}
    influxlog_event(const influxlog_event& rhs) = default;
    influxlog_event(influxlog_event&& rhs) = default;
    influxlog_event& operator=(const influxlog_event& rhs) = default;
    bool operator==(const influxlog_event& rhs) const
    {
        return measurement == rhs.measurement && tags == rhs.tags && fields == rhs.fields && timestamp == rhs.timestamp;
    }
    bool operator!=(const influxlog_event& rhs) const { return !(*this == rhs); }
    virtual ~influxlog_event() override = default;

    virtual octetsstream& pack(octetsstream& os) const override;
    virtual octetsstream& unpack(octetsstream& os) override;

    virtual rpcdata* dup() const override { return new influxlog_event(*this); }
    virtual ostringstream& dump(ostringstream& out) const override;

public:
    std::string measurement;
    std::map<std::string, std::string> tags;
    std::map<std::string, std::string> fields;
    int64_t timestamp = 0;
};

} // namespace bee