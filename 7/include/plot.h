#include <QDateTime>
#include <QVector>
#include <qwt_plot.h>
#include <qwt_plot_curve.h>
#include <qwt_symbol.h>

class Plot : public QwtPlot {
    Q_OBJECT
public:
    explicit Plot(QWidget *parent = nullptr);

    void updatePlot(const QVector<QPair<qint64, double>> &data);

private:
    QwtPlotCurve *curve_;
};