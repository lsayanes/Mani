#pragma once

#include <QDialog>

class QLineEdit;

class PasswordConfigDialog : public QDialog
{
    Q_OBJECT

public:
    explicit PasswordConfigDialog(QWidget *parent = nullptr);

private slots:
    void accept() override;

private:
    QLineEdit *m_passwordEdit = nullptr;
    QLineEdit *m_confirmEdit = nullptr;
};
