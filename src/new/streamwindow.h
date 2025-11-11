#pragma once

#include <QWidget>
#include <QLabel>
#include <QVBoxLayout>

class StreamWindow : public QWidget
{
    Q_OBJECT

public:
    explicit StreamWindow(QWidget *parent = nullptr);
    virtual ~StreamWindow() = default;

    // Общие методы для всех окон трансляций
    virtual int getStreamId() const = 0;
    virtual QString getDisplayId() const = 0;
    virtual bool isActive() const = 0;
    
    // Общие методы управления
    virtual void initialize() = 0;
    virtual void cleanup();
    
    // Общие UI элементы
    void setStreamInfo(const QString &info);
    void setStatus(const QString &status, const QString &color = "#666");
    void showError(const QString &error);

signals:
    void windowClosed(int streamId);
    void errorOccurred(const QString &message);

protected:
    // Общие UI компоненты
    void setupCommonUI();
    void setupCommonConnections();
    
    // Общие обработчики
    void closeEvent(QCloseEvent *event) override;
    
    // Общие виджеты
    QVBoxLayout *m_mainLayout;
    QLabel *m_statusLabel;
    QLabel *m_streamInfoLabel;
    QLabel *m_errorLabel;

private:
    void setupErrorLabel();
};
