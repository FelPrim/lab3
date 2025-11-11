#pragma once

#include <QWidget>
#include <QLabel>
#include <QVBoxLayout>

class StreamVideoDisplayPanel : public QWidget
{
    Q_OBJECT

public:
    explicit StreamVideoDisplayPanel(QWidget *parent = nullptr);
    ~StreamVideoDisplayPanel();

    void setPlaceholderText(const QString &text);
    void showVideo(bool show);
    void setStreamId(int streamId);

public slots:
    void displayFrame(const QImage &frame);
    void clearDisplay();

private:
    void setupUI();
    
    QLabel *m_videoLabel;
    QLabel *m_placeholderLabel;
    QVBoxLayout *m_layout;
};
