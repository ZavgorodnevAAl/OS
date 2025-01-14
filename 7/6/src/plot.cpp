#include "../include/plot.h"
#include <QDateTime>
#include <QDebug>
#include <qwt_plot_grid.h>
#include <qwt_plot_zoomer.h>
#include <qwt_date.h>
#include <qwt_scale_draw.h>
#include <qwt_text.h>
#include <QTimeZone>

class CustomScaleDraw : public QwtScaleDraw
{
public:
    CustomScaleDraw(const QTimeZone& timeZone) : timeZone_(timeZone) {}
    
    QwtText label(double v) const override {
        QDateTime dateTime = QDateTime::fromMSecsSinceEpoch(static_cast<qint64>(v * 1000), timeZone_);
        return dateTime.toString("yyyy-MM-dd hh:mm:ss");
    }
private:
    QTimeZone timeZone_;
};


Plot::Plot(QWidget *parent) : QwtPlot(parent) {
    setAutoReplot(true);

    QwtPlotGrid *grid = new QwtPlotGrid();
    grid->attach(this);

    setAxisTitle(QwtPlot::xBottom, "Time");
    setAxisTitle(QwtPlot::yLeft, "Temperature");

    curve_ = new QwtPlotCurve();
    curve_->attach(this);

    QwtSymbol *symbol = new QwtSymbol(QwtSymbol::Ellipse, QColor(Qt::blue), QColor(Qt::blue), QSize(5, 5));
    curve_->setSymbol(symbol);
    
    
#ifdef QWT_DATE_SCALE_DRAW_AVAILABLE
    setAxisScaleDraw(QwtPlot::xBottom, new QwtDateScaleDraw);
#else
    setAxisScaleDraw(QwtPlot::xBottom, new CustomScaleDraw(QTimeZone::systemTimeZone()));
#endif
    

    QwtPlotZoomer *zoomer = new QwtPlotZoomer(canvas());
    zoomer->setRubberBandPen(QColor(Qt::black));
    zoomer->setTrackerPen(QColor(Qt::black));
}

void Plot::updatePlot(const QVector<QPair<qint64, double>> &data) {

    QVector<double> xValues, yValues;
    for (const auto &point : data) {
        xValues.append(point.first / 1000.0);
        yValues.append(point.second);
    }

    curve_->setSamples(xValues, yValues);
    setAxisScale(QwtPlot::yLeft, 0, 50);
    replot();
}

