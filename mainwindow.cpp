#include "mainwindow.h"
#include "infection.h"

#include <QtCharts/QChart>
#include <QtCharts/QChartView>
#include <QComboBox>
#include <QDoubleSpinBox>
#include <QFormLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QLabel>
#include <QtCharts/QLineSeries>
#include <QMessageBox>
#include <QPainter>
#include <QPushButton>
#include <QScrollArea>
#include <QSlider>
#include <QSpinBox>
#include <QSplitter>
#include <QTableWidget>
#include <QTimer>
#include <QVBoxLayout>
#include <QtCharts/QValueAxis>
#include <algorithm>
#include <cmath>
#include <random>

namespace {

QString grouped(long long value){
    return QLocale(QLocale::Russian).toString(value);
}

QString money(double value){
    return QLocale(QLocale::Russian).toString(value, 'f', 0);
}

QString percentText(double fraction){
    return QLocale(QLocale::Russian).toString(fraction * 100.0, 'f', 1) + " %";
}

QString capitalizedMonth(int month){
    QString name = QString::fromUtf8(monthName(month));
    if (!name.isEmpty()) {
        name[0] = name[0].toUpper();
    }
    return name;
}

QDoubleSpinBox* percentSpin(double value){
    auto* spin = new QDoubleSpinBox;
    spin->setRange(0.0, 100.0);
    spin->setDecimals(1);
    spin->setSingleStep(0.5);
    spin->setSuffix(" %");
    spin->setValue(value);
    return spin;
}

CitySetup defaultCity(int index){
    static const CityType kTypes[] = {CityType::Megapolis, CityType::Medium, CityType::Settlement,
                                      CityType::Medium, CityType::Settlement};
    CitySetup setup;
    setup.type = kTypes[index % 5];
    setup.name = "Город " + std::to_string(index + 1);
    switch (setup.type) {
    case CityType::Megapolis:
        setup.population = 1200000;
        setup.transport = 0.8;
        break;
    case CityType::Medium:
        setup.population = 180000;
        setup.transport = 0.55;
        break;
    case CityType::Settlement:
        setup.population = 18000;
        setup.transport = 0.3;
        break;
    }
    setup.initialSickFraction = 0.02;
    setup.initialImmuneFraction = 0.05;
    return setup;
}

}

MainWindow::MainWindow(){
    buildUi();
    setWindowTitle("Моделирование распространения вирусного заболевания");
    generateCountry();
}

void MainWindow::buildUi(){
    auto* central = new QWidget(this);
    setCentralWidget(central);

    months_ = new QSpinBox;
    months_->setRange(6, 24);
    months_->setValue(12);
    months_->setSuffix(" мес.");

    cityCount_ = new QSpinBox;
    cityCount_->setRange(5, 10);
    cityCount_->setValue(6);

    month_ = new QComboBox;
    for (int i = 0; i < 12; ++i){
        month_->addItem(capitalizedMonth(i));
    }
    month_->setCurrentIndex(8);

    cost_ = new QDoubleSpinBox;
    cost_->setRange(0.01, 1000000);
    cost_->setDecimals(2);
    cost_->setValue(10);
    cost_->setSuffix(" у.е.");

    fund_ = new QDoubleSpinBox;
    fund_->setRange(0, 1e12);
    fund_->setDecimals(0);
    fund_->setValue(5000000);
    fund_->setSuffix(" у.е.");
    fund_->setGroupSeparatorShown(true);

    tax_ = new QDoubleSpinBox;
    tax_->setRange(0, 100000);
    tax_->setDecimals(2);
    tax_->setValue(2);
    tax_->setSuffix(" у.е.");

    sickPay_ = new QDoubleSpinBox;
    sickPay_->setRange(0, 100000);
    sickPay_->setDecimals(2);
    sickPay_->setValue(15);
    sickPay_->setSuffix(" у.е.");

    strategy_ = new QComboBox;
    strategy_->addItem("Ручной ввод");
    strategy_->addItem("Без прививок");
    strategy_->addItem("Поровну по городам");
    strategy_->addItem("Приоритет крупным городам");
    strategy_->addItem("Пропорционально риску");
    strategy_->setCurrentIndex(static_cast<int>(StrategyKind::LargeCityPriority));

    speed_ = new QSlider(Qt::Horizontal);
    speed_->setRange(1, 20);
    speed_->setValue(8);
    speed_->setFixedWidth(140);

    auto* form = new QFormLayout;
    form->addRow("Период M", months_);
    form->addRow("Число городов N", cityCount_);
    form->addRow("Месяц начала", month_);
    form->addRow("Цена прививки", cost_);
    form->addRow("Начальный фонд", fund_);
    form->addRow("Налог работника / нед.", tax_);
    form->addRow("Больничные / нед.", sickPay_);
    form->addRow("Стратегия прививок", strategy_);

    generateBtn_ = new QPushButton("Случайная страна");
    auto* paramsBox = new QGroupBox("Параметры моделирования");
    auto* paramsLayout = new QVBoxLayout(paramsBox);
    paramsLayout->addLayout(form);
    paramsLayout->addWidget(generateBtn_);

    table_ = new QTableWidget(0, 7);
    table_->setHorizontalHeaderLabels({"Город", "Тип", "Население", "Транспорт", "Больные", "Привитые", "Дозы"});
    table_->verticalHeader()->setVisible(false);
    table_->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    table_->horizontalHeader()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
    table_->setSelectionBehavior(QAbstractItemView::SelectRows);
    table_->setMinimumHeight(240);

    auto* citiesBox = new QGroupBox("Города");
    auto* citiesLayout = new QVBoxLayout(citiesBox);
    citiesLayout->addWidget(table_);

    details_ = new QLabel("Выберите город на карте или в таблице.");
    details_->setWordWrap(true);
    details_->setTextFormat(Qt::RichText);
    details_->setAlignment(Qt::AlignTop);
    auto* detailsBox = new QGroupBox("Сведения о городе");
    auto* detailsLayout = new QVBoxLayout(detailsBox);
    detailsLayout->addWidget(details_);

    auto* left = new QWidget;
    auto* leftLayout = new QVBoxLayout(left);
    leftLayout->setContentsMargins(0, 0, 0, 0);
    leftLayout->addWidget(paramsBox);
    leftLayout->addWidget(citiesBox, 1);
    leftLayout->addWidget(detailsBox);
    left->setMinimumWidth(460);

    map_ = new MapView;
    auto* legend = new QLabel(
        "Зелёный круг — население. Красный сектор — доля больных. "
        "Красная обводка — эпидемия (45% и больше). Пунктир — выбранный город.");
    legend->setWordWrap(true);
    auto* mapBox = new QGroupBox("Карта страны");
    auto* mapLayout = new QVBoxLayout(mapBox);
    mapLayout->addWidget(legend);
    mapLayout->addWidget(map_, 1);

    sickSeries_ = new QLineSeries;
    sickSeries_->setName("Больные");
    sickSeries_->setColor(QColor(196, 48, 43));
    thresholdSeries_ = new QLineSeries;
    thresholdSeries_->setName("Порог 45%");
    QPen thresholdPen(QColor(170, 50, 40));
    thresholdPen.setStyle(Qt::DashLine);
    thresholdSeries_->setPen(thresholdPen);
    sickChart_ = new QChart;
    sickChart_->addSeries(sickSeries_);
    sickChart_->addSeries(thresholdSeries_);
    sickChart_->setTitle("Доля заболевших в стране");
    sickChart_->legend()->hide();
    sickChart_->setAnimationOptions(QChart::NoAnimation);
    sickChart_->setMargins(QMargins(4, 4, 4, 4));
    sickX_ = new QValueAxis;
    sickY_ = new QValueAxis;
    sickX_->setTitleText("Неделя");
    sickY_->setTitleText("%");
    sickY_->setRange(0, 100);
    sickX_->setLabelFormat("%.0f");
    sickY_->setLabelFormat("%.0f");
    sickChart_->addAxis(sickX_, Qt::AlignBottom);
    sickChart_->addAxis(sickY_, Qt::AlignLeft);
    sickSeries_->attachAxis(sickX_);
    sickSeries_->attachAxis(sickY_);
    thresholdSeries_->attachAxis(sickX_);
    thresholdSeries_->attachAxis(sickY_);
    auto* sickView = new QChartView(sickChart_);
    sickView->setRenderHint(QPainter::Antialiasing);
    sickView->setMinimumHeight(160);

    fundSeries_ = new QLineSeries;
    fundSeries_->setColor(QColor(36, 99, 168));
    fundChart_ = new QChart;
    fundChart_->addSeries(fundSeries_);
    fundChart_->setTitle("Денежный фонд");
    fundChart_->legend()->hide();
    fundChart_->setAnimationOptions(QChart::NoAnimation);
    fundChart_->setMargins(QMargins(4, 4, 4, 4));
    fundX_ = new QValueAxis;
    fundY_ = new QValueAxis;
    fundX_->setTitleText("Неделя");
    fundY_->setTitleText("у.е.");
    fundX_->setLabelFormat("%.0f");
    fundY_->setLabelFormat("%.0f");
    fundChart_->addAxis(fundX_, Qt::AlignBottom);
    fundChart_->addAxis(fundY_, Qt::AlignLeft);
    fundSeries_->attachAxis(fundX_);
    fundSeries_->attachAxis(fundY_);
    const QFont chartFont = font();
    for (QChart* chart : {sickChart_, fundChart_}) {
        chart->setFont(chartFont);
        chart->setTitleFont(chartFont);
    }
    for (QValueAxis* axis : {sickX_, sickY_, fundX_, fundY_}) {
        axis->setLabelsFont(chartFont);
        axis->setTitleFont(chartFont);
    }
    auto* fundView = new QChartView(fundChart_);
    fundView->setRenderHint(QPainter::Antialiasing);
    fundView->setMinimumHeight(160);

    auto* charts = new QSplitter(Qt::Vertical);
    charts->addWidget(sickView);
    charts->addWidget(fundView);

    auto* right = new QSplitter(Qt::Vertical);
    right->addWidget(mapBox);
    right->addWidget(charts);
    right->setStretchFactor(0, 3);
    right->setStretchFactor(1, 2);

    auto* scroll = new QScrollArea;
    scroll->setWidget(left);
    scroll->setWidgetResizable(true);
    scroll->setFrameShape(QFrame::NoFrame);
    scroll->setMinimumWidth(420);

    auto* split = new QSplitter;
    split->addWidget(scroll);
    split->addWidget(right);
    split->setStretchFactor(0, 0);
    split->setStretchFactor(1, 1);

    startBtn_ = new QPushButton("Старт");
    stepBtn_ = new QPushButton("Шаг");
    runBtn_ = new QPushButton("Пуск");
    resetBtn_ = new QPushButton("Сброс");
    auto* controls = new QHBoxLayout;
    controls->addWidget(startBtn_);
    controls->addWidget(stepBtn_);
    controls->addWidget(runBtn_);
    controls->addWidget(resetBtn_);
    controls->addSpacing(16);
    controls->addWidget(new QLabel("Темп"));
    controls->addWidget(speed_);
    controls->addStretch();

    status_ = new QLabel;
    status_->setWordWrap(true);
    auto* hint = new QLabel(
        "Шаг — одна неделя. Прививка начинает действовать через три недели и не делается болеющим. "
        "Порог эпидемии — 45% больных. Работающих — 65% населения.");
    hint->setWordWrap(true);
    hint->setStyleSheet("color: #4a5562;");

    auto* root = new QVBoxLayout(central);
    root->addWidget(split, 1);
    root->addLayout(controls);
    root->addWidget(status_);
    root->addWidget(hint);

    timer_ = new QTimer(this);
    connect(timer_, &QTimer::timeout, this, &MainWindow::stepOnce);
    connect(speed_, &QSlider::valueChanged, this, [this](int value) {
        timer_->setInterval(std::max(40, 1050 - value * 48));
    });
    timer_->setInterval(1050 - speed_->value() * 48);

    connect(generateBtn_, &QPushButton::clicked, this, &MainWindow::generateCountry);
    connect(startBtn_, &QPushButton::clicked, this, &MainWindow::startSimulation);
    connect(stepBtn_, &QPushButton::clicked, this, &MainWindow::stepOnce);
    connect(runBtn_, &QPushButton::clicked, this, &MainWindow::toggleRun);
    connect(resetBtn_, &QPushButton::clicked, this, &MainWindow::resetSimulation);
    connect(cityCount_, QOverload<int>::of(&QSpinBox::valueChanged), this, &MainWindow::onCityCountChanged);
    connect(strategy_, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [this](int) {
        syncDoseEditors();
    });
    connect(map_, &MapView::citySelected, this, &MainWindow::onCitySelected);
    connect(table_, &QTableWidget::currentCellChanged, this, [this](int row, int, int, int) {
        if (updating_ || row < 0) {
            return;
        }
        selected_ = row;
        if (simulation_.country().count() > 0) {
            map_->setState(simulation_.country(), selected_);
            updateDetails();
        }
    });
    connect(table_, &QTableWidget::itemChanged, this, [this] { preview(); });

    const auto previewOn = [this] { preview(); };
    connect(months_, QOverload<int>::of(&QSpinBox::valueChanged), this, previewOn);
    connect(month_, QOverload<int>::of(&QComboBox::currentIndexChanged), this, previewOn);
    connect(cost_, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, previewOn);
    connect(fund_, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, previewOn);
    connect(tax_, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, previewOn);
    connect(sickPay_, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, previewOn);

    setStyleSheet(
        "QGroupBox { font-weight: 600; border: 1px solid #d5dbe3; border-radius: 8px;"
        " margin-top: 12px; padding: 8px; background: #ffffff; }"
        "QGroupBox::title { subcontrol-origin: margin; left: 10px; padding: 0 4px; }"
        "QPushButton { padding: 6px 14px; }"
        "QMainWindow, QWidget#central { background: #f3f5f8; }");
    central->setObjectName("central");
}

void MainWindow::addCityRow(const CitySetup& setup) {
    const int row = table_->rowCount();
    table_->insertRow(row);

    auto* name = new QTableWidgetItem(QString::fromStdString(setup.name));
    table_->setItem(row, 0, name);

    auto* type = new QComboBox;
    type->addItem("Мегаполис", static_cast<int>(CityType::Megapolis));
    type->addItem("Средний город", static_cast<int>(CityType::Medium));
    type->addItem("Посёлок", static_cast<int>(CityType::Settlement));
    const int typeIndex = type->findData(static_cast<int>(setup.type));
    type->setCurrentIndex(std::max(0, typeIndex));
    table_->setCellWidget(row, 1, type);

    auto* population = new QSpinBox;
    population->setRange(1000, 5000000);
    population->setSingleStep(1000);
    population->setGroupSeparatorShown(true);
    population->setValue(std::clamp(setup.population, 1000, 5000000));
    table_->setCellWidget(row, 2, population);

    auto* transport = new QDoubleSpinBox;
    transport->setRange(0.0, 1.0);
    transport->setDecimals(2);
    transport->setSingleStep(0.05);
    transport->setValue(std::clamp(setup.transport, 0.0, 1.0));
    table_->setCellWidget(row, 3, transport);

    table_->setCellWidget(row, 4, percentSpin(setup.initialSickFraction * 100.0));
    table_->setCellWidget(row, 5, percentSpin(setup.initialImmuneFraction * 100.0));

    auto* doses = new QSpinBox;
    doses->setRange(0, 5000000);
    doses->setGroupSeparatorShown(true);
    doses->setValue(0);
    table_->setCellWidget(row, 6, doses);

    const auto previewOn = [this] { preview(); };
    connect(type, QOverload<int>::of(&QComboBox::currentIndexChanged), this, previewOn);
    connect(population, QOverload<int>::of(&QSpinBox::valueChanged), this, previewOn);
    connect(transport, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, previewOn);
    connect(qobject_cast<QDoubleSpinBox*>(table_->cellWidget(row, 4)),
            QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, previewOn);
    connect(qobject_cast<QDoubleSpinBox*>(table_->cellWidget(row, 5)),
            QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, previewOn);
}

void MainWindow::fillTable(const std::vector<CitySetup>& cities) {
    updating_ = true;
    table_->setRowCount(0);
    for (const CitySetup& setup : cities) {
        addCityRow(setup);
    }
    cityCount_->blockSignals(true);
    cityCount_->setValue(static_cast<int>(cities.size()));
    cityCount_->blockSignals(false);
    updating_ = false;
    syncDoseEditors();
}

std::optional<MainWindow::FormData> MainWindow::readForm(QString* error) const {
    FormData form;
    form.params.months = months_->value();
    form.params.startMonth = month_->currentIndex();
    form.params.vaccineCost = cost_->value();
    form.params.initialFund = fund_->value();
    form.params.taxPerWorker = tax_->value();
    form.params.sickPayPerWorker = sickPay_->value();
    form.params.strategy = currentStrategy();
    form.params.seed = 1;

    if (table_->rowCount() < 5 || table_->rowCount() > 10) {
        if (error) {
            *error = "Число городов должно быть от 5 до 10.";
        }
        return std::nullopt;
    }

    for (int row = 0; row < table_->rowCount(); ++row) {
        CitySetup setup;
        const auto* nameItem = table_->item(row, 0);
        setup.name = nameItem ? nameItem->text().trimmed().toStdString() : "";
        if (setup.name.empty()) {
            if (error) {
                *error = QString("Укажите название города в строке %1.").arg(row + 1);
            }
            return std::nullopt;
        }
        auto* type = qobject_cast<QComboBox*>(table_->cellWidget(row, 1));
        auto* population = qobject_cast<QSpinBox*>(table_->cellWidget(row, 2));
        auto* transport = qobject_cast<QDoubleSpinBox*>(table_->cellWidget(row, 3));
        auto* sick = qobject_cast<QDoubleSpinBox*>(table_->cellWidget(row, 4));
        auto* immune = qobject_cast<QDoubleSpinBox*>(table_->cellWidget(row, 5));
        if (!type || !population || !transport || !sick || !immune) {
            if (error) {
                *error = "Таблица городов заполнена неполностью.";
            }
            return std::nullopt;
        }
        if (sick->value() + immune->value() > 100.0 + 1e-6) {
            if (error) {
                *error = QString("В строке %1 сумма больных и привитых больше 100%.")
                             .arg(row + 1);
            }
            return std::nullopt;
        }
        setup.type = static_cast<CityType>(type->currentData().toInt());
        setup.population = population->value();
        setup.transport = transport->value();
        setup.initialSickFraction = sick->value() / 100.0;
        setup.initialImmuneFraction = immune->value() / 100.0;
        form.cities.push_back(std::move(setup));
    }
    return form;
}

std::vector<int> MainWindow::readDoses() const {
    std::vector<int> doses;
    doses.reserve(static_cast<std::size_t>(table_->rowCount()));
    for (int row = 0; row < table_->rowCount(); ++row) {
        auto* spin = qobject_cast<QSpinBox*>(table_->cellWidget(row, 6));
        doses.push_back(spin ? spin->value() : 0);
    }
    return doses;
}

StrategyKind MainWindow::currentStrategy() const {
    return static_cast<StrategyKind>(strategy_->currentIndex());
}

void MainWindow::generateCountry() {
    if (started_) {
        return;
    }
    std::mt19937 rng{std::random_device{}()};
    const std::vector<CitySetup> cities = Simulation::randomCountry(cityCount_->value(), rng);
    fillTable(cities);
    if (!cities.empty()) {
        selected_ = 0;
        table_->setCurrentCell(0, 0);
    }
    preview();
}

bool MainWindow::startSimulation() {
    if (started_) {
        return true;
    }
    QString error;
    const std::optional<FormData> form = readForm(&error);
    if (!form) {
        QMessageBox::warning(this, "Параметры", error);
        return false;
    }
    std::mt19937 rng{std::random_device{}()};
    FormData ready = *form;
    ready.params.seed = rng();
    updating_ = true;
    simulation_.reset(ready.cities, ready.params);
    simulation_.setManualDoses(readDoses());
    simulation_.setStrategy(ready.params.strategy);
    started_ = true;
    selected_ = simulation_.country().count() > 0 ? 0 : -1;
    lastStepDoses_ = 0;
    setEditingEnabled(false);
    syncDoseEditors();
    if (selected_ >= 0) {
        table_->setCurrentCell(selected_, 0);
    }
    updating_ = false;
    stepBtn_->setEnabled(true);
    runBtn_->setEnabled(true);
    runBtn_->setText("Пуск");
    refreshView();
    return true;
}

void MainWindow::stepOnce() {
    if (!started_) {
        if (!startSimulation()) {
            timer_->stop();
            runBtn_->setText("Пуск");
            return;
        }
    }
    if (simulation_.finished()) {
        timer_->stop();
        runBtn_->setText("Пуск");
        runBtn_->setEnabled(false);
        stepBtn_->setEnabled(false);
        return;
    }

    simulation_.setManualDoses(readDoses());
    simulation_.setStrategy(currentStrategy());
    const StepResult result = simulation_.step();
    lastStepDoses_ = result.doses;

    if (currentStrategy() != StrategyKind::Manual) {
        updating_ = true;
        for (int row = 0; row < table_->rowCount() && row < static_cast<int>(result.dosesPerCity.size()); ++row){
            if (auto* spin = qobject_cast<QSpinBox*>(table_->cellWidget(row, 6))) {
                spin->setValue(result.dosesPerCity[static_cast<std::size_t>(row)]);
            }
        }
        updating_ = false;
    }
    refreshView();
    if (result.finished){
        timer_->stop();
        runBtn_->setText("Пуск");
        runBtn_->setEnabled(false);
        stepBtn_->setEnabled(false);
    }
}

void MainWindow::toggleRun(){
    if (timer_->isActive()) {
        timer_->stop();
        runBtn_->setText("Пуск");
        return;
    }
    if (!started_) {
        if (!startSimulation()) {
            return;
        }
    }
    if (simulation_.finished()) {
        return;
    }
    timer_->start();
    runBtn_->setText("Пауза");
}

void MainWindow::resetSimulation(){
    timer_->stop();
    started_ = false;
    lastStepDoses_ = 0;
    runBtn_->setText("Пуск");
    runBtn_->setEnabled(true);
    stepBtn_->setEnabled(true);
    setEditingEnabled(true);
    syncDoseEditors();
    sickSeries_->clear();
    fundSeries_->clear();
    thresholdSeries_->clear();
    preview();
}

void MainWindow::preview(){
    if (started_ || updating_) {
        return;
    }
    QString error;
    const std::optional<FormData> form = readForm(&error);
    if (!form) {
        return;
    }
    updating_ = true;
    simulation_.reset(form->cities, form->params);
    if (selected_ >= simulation_.country().count()) {
        selected_ = simulation_.country().count() > 0 ? 0 : -1;
    }
    updating_ = false;
    lastStepDoses_ = 0;
    refreshView();
}

void MainWindow::refreshView(){
    if (simulation_.country().count() > 0 && selected_ < 0){
        selected_ = 0;
    }
    map_->setState(simulation_.country(), selected_);
    updateDetails();
    updateStatus(lastStepDoses_);
    if (started_) {
        rebuildCharts();
        for (int row = 0; row < table_->rowCount() && row < simulation_.country().count(); ++row){
            auto* item = table_->item(row, 0);
            if (!item) {
                continue;
            }
            const bool epidemic = simulation_.country().city(row).isEpidemic();
            item->setBackground(epidemic ? QColor(255, 220, 214) : QColor(255, 255, 255));
        }
    }
}

void MainWindow::rebuildCharts(){
    sickSeries_->clear();
    fundSeries_->clear();
    double minFund = 0;
    double maxFund = 0;
    bool haveFund = false;
    for (const HistoryPoint& point : simulation_.history()) {
        sickSeries_->append(point.week, point.sickFraction * 100.0);
        fundSeries_->append(point.week, point.fund);
        if (!haveFund) {
            minFund = maxFund = point.fund;
            haveFund = true;
        } else {
            minFund = std::min(minFund, point.fund);
            maxFund = std::max(maxFund, point.fund);
        }
    }
    const int weeks = std::max(1, simulation_.totalWeeks());
    sickX_->setRange(0, weeks);
    fundX_->setRange(0, weeks);
    thresholdSeries_->clear();
    thresholdSeries_->append(0, 45);
    thresholdSeries_->append(weeks, 45);
    if (!haveFund) {
        minFund = 0;
        maxFund = 1;
    }
    if (std::abs(maxFund - minFund) < 1.0) {
        minFund -= 1.0;
        maxFund += 1.0;
    }
    const double pad = (maxFund - minFund) * 0.1;
    fundY_->setRange(minFund - pad, maxFund + pad);
}

void MainWindow::updateDetails(){
    if (selected_ < 0 || selected_ >= simulation_.country().count()){
        details_->setText("Выберите город на карте или в таблице.");
        return;
    }
    const City& city = simulation_.country().city(selected_);
    const QString name = QString::fromStdString(city.name()).toHtmlEscaped();
    const QString epidemic = city.isEpidemic()
                                 ? "<span style='color:#b01818;'><b>да</b></span>"
                                 : "нет";
    details_->setText(
        QString("<b>%1</b> (%2)<br>"
                "Население: %3<br>"
                "Больные: %4 (%5)<br>"
                "Здоровые: %6<br>"
                "Иммунные: %7<br>"
                "Ждут действия прививки: %8<br>"
                "Восприимчивые: %9<br>"
                "Транспорт: %10<br>"
                "Эпидемия: %11")
            .arg(name)
            .arg(QString::fromStdString(city.typeName()))
            .arg(grouped(city.population()))
            .arg(grouped(city.sick()))
            .arg(percentText(city.sickFraction()))
            .arg(grouped(city.healthy()))
            .arg(grouped(city.immune()))
            .arg(grouped(city.pendingVaccines()))
            .arg(grouped(city.susceptible()))
            .arg(QString::number(city.transport(), 'f', 2))
            .arg(epidemic));
}

void MainWindow::updateStatus(int dosesThisStep){
    const int week = simulation_.active() ? simulation_.week() : 0;
    const int total = simulation_.active() ? simulation_.totalWeeks() : months_->value() * 4;
    const int month = simulation_.active() ? simulation_.calendarMonth() : month_->currentIndex();
    const double fraction = simulation_.active() ? simulation_.country().sickFraction() : 0;
    const double fundValue = simulation_.active() ? simulation_.country().fund() : fund_->value();
    const int large = simulation_.active() ? simulation_.country().epidemicLargeCityCount() : 0;
    QString text = QString("Неделя %1 из %2 · %3 · больные в стране %4 · фонд %5 у.е. · "
                           "эпидемия в крупных городах: %6")
                       .arg(week)
                       .arg(total)
                       .arg(capitalizedMonth(month))
                       .arg(percentText(fraction))
                       .arg(money(fundValue))
                       .arg(large);
    if (started_ && week > 0) {
        text += QString(" · прививок на шаге: %1").arg(grouped(dosesThisStep));
    }
    if (started_ && simulation_.finished()) {
        text += " · моделирование завершено";
    }
    status_->setText(text);
}

void MainWindow::setEditingEnabled(bool enabled){
    months_->setEnabled(enabled);
    cityCount_->setEnabled(enabled);
    month_->setEnabled(enabled);
    cost_->setEnabled(enabled);
    fund_->setEnabled(enabled);
    tax_->setEnabled(enabled);
    sickPay_->setEnabled(enabled);
    generateBtn_->setEnabled(enabled);
    startBtn_->setEnabled(enabled);
    for (int row = 0; row < table_->rowCount(); ++row) {
        if (auto* item = table_->item(row, 0)) {
            Qt::ItemFlags flags = Qt::ItemIsEnabled | Qt::ItemIsSelectable;
            if (enabled) {
                flags |= Qt::ItemIsEditable;
            }
            item->setFlags(flags);
            if (enabled) {
                item->setBackground(QColor(255, 255, 255));
            }
        }
        for (int column : {1, 2, 3, 4, 5}) {
            if (auto* widget = table_->cellWidget(row, column)) {
                widget->setEnabled(enabled);
            }
        }
    }
}

void MainWindow::syncDoseEditors() {
    const bool manual = currentStrategy() == StrategyKind::Manual;
    for (int row = 0; row < table_->rowCount(); ++row) {
        auto* spin = qobject_cast<QSpinBox*>(table_->cellWidget(row, 6));
        if (!spin) {
            continue;
        }
        spin->setReadOnly(!manual);
        spin->setFocusPolicy(manual ? Qt::WheelFocus : Qt::NoFocus);
        if (!manual && !started_) {
            spin->setValue(0);
        }
    }
}

void MainWindow::onCitySelected(int index) {
    if (index < 0 || index >= table_->rowCount()) {
        return;
    }
    selected_ = index;
    updating_ = true;
    table_->setCurrentCell(index, 0);
    updating_ = false;
    if (simulation_.country().count() > 0) {
        map_->setState(simulation_.country(), selected_);
        updateDetails();
    }
}

void MainWindow::onCityCountChanged(int count) {
    if (started_ || updating_) {
        return;
    }
    updating_ = true;
    while (table_->rowCount() > count) {
        table_->removeRow(table_->rowCount() - 1);
    }
    while (table_->rowCount() < count) {
        addCityRow(defaultCity(table_->rowCount()));
    }
    updating_ = false;
    syncDoseEditors();
    preview();
}
