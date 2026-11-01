#pragma once
#include "simulation.h"
#include "mapview.h"
#include <QMainWindow>
#include <optional>

class QChart;
class QComboBox;
class QDoubleSpinBox;
class QLabel;
class QLineSeries;
class QPushButton;
class QSlider;
class QSpinBox;
class QTableWidget;
class QTimer;
class QValueAxis;

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    MainWindow();

private:
    struct FormData {
        Parameters params;
        std::vector<CitySetup> cities;
    };

    void buildUi();
    void addCityRow(const CitySetup& setup);
    void fillTable(const std::vector<CitySetup>& cities);
    std::optional<FormData> readForm(QString* error) const;
    std::vector<int> readDoses() const;
    StrategyKind currentStrategy() const;

    void generateCountry();
    bool startSimulation();
    void stepOnce();
    void toggleRun();
    void resetSimulation();
    void preview();
    void refreshView();
    void rebuildCharts();
    void updateDetails();
    void updateStatus(int dosesThisStep);
    void setEditingEnabled(bool enabled);
    void syncDoseEditors();
    void onCitySelected(int index);
    void onCityCountChanged(int count);

    QSpinBox* months_ = nullptr;
    QSpinBox* cityCount_ = nullptr;
    QComboBox* month_ = nullptr;
    QDoubleSpinBox* cost_ = nullptr;
    QDoubleSpinBox* fund_ = nullptr;
    QDoubleSpinBox* tax_ = nullptr;
    QDoubleSpinBox* sickPay_ = nullptr;
    QComboBox* strategy_ = nullptr;
    QSlider* speed_ = nullptr;
    QTableWidget* table_ = nullptr;
    QLabel* details_ = nullptr;
    QLabel* status_ = nullptr;
    QPushButton* generateBtn_ = nullptr;
    QPushButton* startBtn_ = nullptr;
    QPushButton* stepBtn_ = nullptr;
    QPushButton* runBtn_ = nullptr;
    QPushButton* resetBtn_ = nullptr;
    MapView* map_ = nullptr;
    QChart* sickChart_ = nullptr;
    QChart* fundChart_ = nullptr;
    QLineSeries* sickSeries_ = nullptr;
    QLineSeries* thresholdSeries_ = nullptr;
    QLineSeries* fundSeries_ = nullptr;
    QValueAxis* sickX_ = nullptr;
    QValueAxis* sickY_ = nullptr;
    QValueAxis* fundX_ = nullptr;
    QValueAxis* fundY_ = nullptr;
    QTimer* timer_ = nullptr;

    Simulation simulation_;
    int selected_ = -1;
    bool started_ = false;
    bool updating_ = false;
    int lastStepDoses_ = 0;
};
