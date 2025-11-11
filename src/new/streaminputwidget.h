#pragma once

#include <QWidget>
#include <QLineEdit>
#include <QPushButton>
#include <QLabel>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QRegExpValidator>

class StreamIdInputWidget : public QWidget
{
    Q_OBJECT

public:
    explicit StreamIdInputWidget(QWidget *parent = nullptr);

    QString getStreamId() const { return m_streamIdInput->text().toUpper(); }
    bool isValid() const { return m_streamIdInput->hasAcceptableInput(); }
    void clear();
    void setEnabled(bool enabled);

signals:
    void streamIdChanged(const QString &streamId);
    void joinRequested(const QString &streamId);

private slots:
    void onTextChanged(const QString &text);
    void onJoinClicked();

private:
    void setupUI();
    void setupConnections();

    QLineEdit *m_streamIdInput;
    QPushButton *m_joinButton;
    QRegExpValidator *m_validator;
    
    static const QRegExp STREAM_ID_REGEX;
};
