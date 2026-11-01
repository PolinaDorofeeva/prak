#pragma once
#include "country.h"
#include <QGraphicsView>

class QGraphicsScene;

class MapView : public QGraphicsView {
    Q_OBJECT

public:
    explicit MapView(QWidget* parent = nullptr);
    void setState(const Country& country, int selected);

signals:
    void citySelected(int index);

protected:
    void resizeEvent(QResizeEvent* event) override;

private:
    void rebuild(const Country& country, int selected);
    void refit();

    QGraphicsScene* scene_ = nullptr;
    int shownCount_ = -1;
};
