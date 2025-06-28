#include "metric.h"
#include "metric_exporter.h"

namespace bee
{

void influx_metric::export_to(metric_exporter& exp) const
{
    exp.write(this);
}

void prometheus_metric::export_to(metric_exporter& exp) const
{
    exp.write(this);
}

} // namespace bee