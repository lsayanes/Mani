#include "ui/PasswordConfigDialog.h"

#include "util/AppLock.h"

#include <QDialogButtonBox>
#include <QFormLayout>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QPushButton>
#include <QVBoxLayout>

PasswordConfigDialog::PasswordConfigDialog(QWidget *parent)
    : QDialog(parent)
{
    setWindowTitle(tr("Bloqueo de la app"));
    resize(380, 200);

    auto *intro = new QLabel(
        AppLock::isEnabled() ? tr("Cambia la contraseña de bloqueo o desactivala.")
                             : tr("Define una contraseña para bloquear Mani al iniciar."),
        this);
    intro->setWordWrap(true);

    m_passwordEdit = new QLineEdit(this);
    m_passwordEdit->setEchoMode(QLineEdit::Password);
    m_passwordEdit->setPlaceholderText(tr("Nueva contraseña"));

    m_confirmEdit = new QLineEdit(this);
    m_confirmEdit->setEchoMode(QLineEdit::Password);
    m_confirmEdit->setPlaceholderText(tr("Confirmar contraseña"));

    auto *form = new QFormLayout;
    form->addRow(tr("Contraseña"), m_passwordEdit);
    form->addRow(tr("Confirmar"), m_confirmEdit);

    auto *disableButton = new QPushButton(tr("Desactivar bloqueo"), this);
    disableButton->setVisible(AppLock::isEnabled());
    connect(disableButton, &QPushButton::clicked, this, [this]() {
        AppLock::clearPassword();
        QMessageBox::information(this, tr("Bloqueo desactivado"), tr("Mani ya no pedira contraseña al iniciar."));
        accept();
    });

    auto *buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    connect(buttons, &QDialogButtonBox::accepted, this, &PasswordConfigDialog::accept);
    connect(buttons, &QDialogButtonBox::rejected, this, &PasswordConfigDialog::reject);

    auto *layout = new QVBoxLayout(this);
    layout->addWidget(intro);
    layout->addLayout(form);
    if (AppLock::isEnabled()) {
        layout->addWidget(disableButton);
    }
    layout->addWidget(buttons);
}

void PasswordConfigDialog::accept()
{
    const QString password = m_passwordEdit->text();
    const QString confirm = m_confirmEdit->text();

    if (password.length() < 4) {
        QMessageBox::warning(this, tr("Contraseña invalida"),
                             tr("La contraseña debe tener al menos 4 caracteres."));
        return;
    }

    if (password != confirm) {
        QMessageBox::warning(this, tr("Contraseña invalida"), tr("Las contraseñas no coinciden."));
        return;
    }

    if (!AppLock::setPassword(password)) {
        QMessageBox::warning(this, tr("Error"), tr("No se pudo guardar la contraseña."));
        return;
    }

    QDialog::accept();
}
