#include "mapview.h"
#include <QGraphicsItem>
#include <QGraphicsScene>
#include <QGraphicsSceneMouseEvent>
#include <QPainter>
#include <QResizeEvent>
#include <QStyleOptionGraphicsItem>
#include <algorithm>
#include <cmath>
#include <functional>

constexpr double kPi = 3.14159265358979323846;

namespace{

class CityNode : public QGraphicsItem{
public:
    CityNode(int index, QString name, double radius, double sickFraction, bool epidemic, bool selected,
             std::function<void(int)> onClick)
        : index_(index), name_(std::move(name)), radius_(radius), sickFraction_(sickFraction),
        epidemic_(epidemic), selected_(selected), onClick_(std::move(onClick)) {}

    void apply(QString name, double radius, double sickFraction, bool epidemic, bool selected) {
        prepareGeometryChange();
        name_ = std::move(name);
        radius_ = radius;
        sickFraction_ = sickFraction;
        epidemic_ = epidemic;
        selected_ = selected;
        update();
    }

    QRectF boundingRect() const override{
        return QRectF(-radius_ - 40, -radius_ - 10, radius_ * 2 + 80, radius_ * 2 + 36);
    }

    void paint(QPainter* painter, const QStyleOptionGraphicsItem*, QWidget*) override{
        painter->setRenderHint(QPainter::Antialiasing);
        const QRectF circle(-radius_, -radius_, radius_ * 2, radius_ * 2);
        const QColor base = epidemic_ ? QColor(255, 228, 206) : QColor(214, 236, 220);
        const QColor sick = epidemic_ ? QColor(176, 32, 32) : QColor(205, 74, 58);
        QPen outline(epidemic_ ? QColor(164, 24, 24) : QColor(68, 96, 82), epidemic_ ? 3.4 : 1.3);

        painter->setPen(outline);
        painter->setBrush(base);
        painter->drawEllipse(circle);

        if (sickFraction_ > 0.0) {
            painter->setPen(Qt::NoPen);
            painter->setBrush(sick);
            const int span = -static_cast<int>(std::lround(std::clamp(sickFraction_, 0.0, 1.0) * 360.0 * 16.0));
            painter->drawPie(circle, 90 * 16, span);
            painter->setPen(outline);
            painter->setBrush(Qt::NoBrush);
            painter->drawEllipse(circle);
        }

        if (selected_) {
            painter->setPen(QPen(QColor(32, 96, 180), 2.0, Qt::DashLine));
            painter->setBrush(Qt::NoBrush);
            painter->drawEllipse(circle.adjusted(-6, -6, 6, 6));
        }

        painter->setPen(QColor(28, 32, 36));
        QFont font = painter->font();
        font.setPointSizeF(9);
        painter->setFont(font);
        painter->drawText(QRectF(-radius_ - 36, radius_ + 4, radius_ * 2 + 72, 20),
                          Qt::AlignHCenter | Qt::AlignTop, name_);
    }

protected:
    void mousePressEvent(QGraphicsSceneMouseEvent* event) override{
        if (onClick_) {
            onClick_(index_);
        }
        QGraphicsItem::mousePressEvent(event);
    }

private:
    int index_;
    QString name_;
    double radius_;
    double sickFraction_;
    bool epidemic_;
    bool selected_;
    std::function<void(int)> onClick_;
};

}

MapView::MapView(QWidget* parent) : QGraphicsView(parent){
    scene_ = new QGraphicsScene(this);
    setScene(scene_);
    setRenderHint(QPainter::Antialiasing);
    setBackgroundBrush(QColor(244, 247, 251));
    setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    setFrameShape(QFrame::NoFrame);
    setMinimumHeight(280);
}

void MapView::setState(const Country& country, int selected){
    if (country.count() != shownCount_) {
        rebuild(country, selected);
        return;
    }
    int maxPop = 1;
    for (int i = 0; i < country.count(); ++i) {
        maxPop = std::max(maxPop, country.city(i).population());
    }
    const auto nodes = scene_->items();
    for (QGraphicsItem* item : nodes) {
        auto* node = dynamic_cast<CityNode*>(item);
        if (!node) {
            continue;
        }
        const int index = node->data(0).toInt();
        if (index < 0 || index >= country.count()) {
            continue;
        }
        const City& city = country.city(index);
        const double radius = 18.0 + 54.0 * std::sqrt(static_cast<double>(city.population()) / maxPop);
        node->apply(QString::fromStdString(city.name()), radius, city.sickFraction(), city.isEpidemic(),
                    index == selected);
        const QString tip = QString("%1\nБольные: %2%\n%3")
                                .arg(QString::fromStdString(city.name()))
                                .arg(city.sickFraction() * 100.0, 0, 'f', 1)
                                .arg(city.isEpidemic() ? "Эпидемия" : "Эпидемии нет");
        node->setToolTip(tip);
    }
    const QRectF bounds = scene_->itemsBoundingRect().adjusted(-28, -28, 28, 28);
    if (bounds != scene_->sceneRect()) {
        scene_->setSceneRect(bounds);
        refit();
    }
}

void MapView::rebuild(const Country& country, int selected){
    scene_->clear();
    shownCount_ = country.count();
    const int n = country.count();
    if (n <= 0) {
        scene_->setSceneRect(-200, -140, 400, 280);
        refit();
        return;
    }

    int maxPop = 1;
    for (int i = 0; i < n; ++i){
        maxPop = std::max(maxPop, country.city(i).population());
    }

    const double layoutX = 230.0;
    const double layoutY = 150.0;
    for (int i = 0; i < n; ++i){
        const City& city = country.city(i);
        const double angle = (2.0 * kPi * i) / n - kPi / 2.0;
        const double radius = 18.0 + 54.0 * std::sqrt(static_cast<double>(city.population()) / maxPop);
        auto* node = new CityNode(
            i, QString::fromStdString(city.name()), radius, city.sickFraction(), city.isEpidemic(),
            i == selected, [this](int index) { emit citySelected(index); });
        node->setData(0, i);
        node->setPos(layoutX * std::cos(angle), layoutY * std::sin(angle));
        node->setCursor(Qt::PointingHandCursor);
        const QString tip = QString("%1\nБольные: %2%\n%3")
                                .arg(QString::fromStdString(city.name()))
                                .arg(city.sickFraction() * 100.0, 0, 'f', 1)
                                .arg(city.isEpidemic() ? "Эпидемия" : "Эпидемии нет");
        node->setToolTip(tip);
        scene_->addItem(node);
    }
    scene_->setSceneRect(scene_->itemsBoundingRect().adjusted(-28, -28, 28, 28));
    refit();
}

void MapView::resizeEvent(QResizeEvent* event){
    QGraphicsView::resizeEvent(event);
    refit();
}

void MapView::refit() {
    if (!scene_ || scene_->items().isEmpty()){
        return;
    }
    fitInView(scene_->sceneRect(), Qt::KeepAspectRatio);
}
