#include "loadtemplatedialog.h"
#include <QDialogButtonBox>
#include <QFont>
#include <QListWidget>
#include <QListWidgetItem>
#include <QPushButton>
#include <QVBoxLayout>

LoadTemplateDialog::LoadTemplateDialog(const TemplateStore &store, QWidget *parent)
    : QDialog(parent)
{
    setWindowTitle("Load Template");
    setMinimumWidth(280);
    setMinimumHeight(360);

    m_list = new QListWidget(this);

    // Helper: add a non-selectable section header
    auto addHeader = [this](const QString &text) {
        auto *item = new QListWidgetItem(text, m_list);
        QFont f = item->font();
        f.setBold(true);
        item->setFont(f);
        item->setFlags(Qt::NoItemFlags);
        item->setBackground(m_list->palette().midlight());
    };

    addHeader("  System Templates");
    for (const QString &name : store.systemNames())
        m_list->addItem("  " + name);

    const QStringList userNames = store.userNames();
    if (!userNames.isEmpty()) {
        addHeader("  User Templates");
        for (const QString &name : userNames)
            m_list->addItem("  " + name);
    }

    auto *buttons = new QDialogButtonBox(
        QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    buttons->button(QDialogButtonBox::Ok)->setEnabled(false);

    connect(m_list, &QListWidget::itemSelectionChanged, this, [this, buttons]() {
        const bool hasSelection = !m_list->selectedItems().isEmpty();
        buttons->button(QDialogButtonBox::Ok)->setEnabled(hasSelection);
    });
    connect(m_list, &QListWidget::itemDoubleClicked, this, [this](QListWidgetItem *item) {
        if (item->flags() & Qt::ItemIsSelectable)
            accept();
    });
    connect(buttons, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);

    auto *layout = new QVBoxLayout(this);
    layout->addWidget(m_list);
    layout->addWidget(buttons);
}

QString LoadTemplateDialog::selectedName() const
{
    const auto items = m_list->selectedItems();
    if (items.isEmpty()) return {};
    return items.first()->text().trimmed();
}
