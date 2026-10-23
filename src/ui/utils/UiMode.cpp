#include "include/ui/utils/UiMode.h"

#include <QAction>
#include <QFormLayout>
#include <QLabel>
#include <QMenu>
#include <QStackedWidget>
#include <QTabWidget>

#include "include/global/Configs.hpp"

namespace UiMode {
    namespace {
        constexpr auto kLevelProperty = "uiLevel";
        constexpr auto kWantedProperty = "uiWanted";

        bool tagged(const QObject *object) {
            return object != nullptr && object->property(kLevelProperty).toString() == QLatin1String("advanced");
        }

        QFormLayout *formHolding(QLayout *layout, QWidget *widget) {
            if (!layout) return nullptr;
            if (auto *form = qobject_cast<QFormLayout *>(layout); form && form->indexOf(widget) >= 0) return form;
            for (int i = 0; i < layout->count(); ++i) {
                if (auto *found = formHolding(layout->itemAt(i)->layout(), widget)) return found;
            }
            return nullptr;
        }

        QFormLayout *formOf(const QWidget *widget) {
            QWidget *parent = widget->parentWidget();
            return parent ? formHolding(parent->layout(), const_cast<QWidget *>(widget)) : nullptr;
        }

        QWidget *rowLabel(const QWidget *widget) {
            auto *form = formOf(widget);
            return form ? form->labelForField(const_cast<QWidget *>(widget)) : nullptr;
        }

        QWidget *rowField(const QLabel *label) {
            if (label->buddy()) return label->buddy();
            auto *form = formOf(label);
            if (!form) return nullptr;
            int row = -1;
            QFormLayout::ItemRole role;
            form->getWidgetPosition(const_cast<QLabel *>(label), &row, &role);
            if (row < 0 || role != QFormLayout::LabelRole) return nullptr;
            auto *item = form->itemAt(row, QFormLayout::FieldRole);
            return item ? item->widget() : nullptr;
        }

        QTabWidget *tabsOf(QWidget *page) {
            auto *stack = qobject_cast<QStackedWidget *>(page->parentWidget());
            auto *tabs = stack ? qobject_cast<QTabWidget *>(stack->parentWidget()) : nullptr;
            return tabs && tabs->indexOf(page) >= 0 ? tabs : nullptr;
        }

        bool shown(QWidget *widget) {
            if (auto *tabs = tabsOf(widget)) return tabs->isTabVisible(tabs->indexOf(widget));
            return !widget->isHidden();
        }

        // QFormLayout::setRowVisible() is Qt 6.4+, so a form field takes its label along by hand.
        void place(QWidget *widget, bool visible) {
            if (auto *tabs = tabsOf(widget)) {
                tabs->setTabVisible(tabs->indexOf(widget), visible);
                return;
            }
            widget->setVisible(visible);
            if (auto *label = rowLabel(widget)) label->setVisible(visible);
        }

        // An item never passed to SetVisible() keeps the visibility it had when Apply() first saw it.
        bool wanted(QObject *object, bool current) {
            const QVariant value = object->property(kWantedProperty);
            if (value.isValid()) return value.toBool();
            object->setProperty(kWantedProperty, current);
            return current;
        }

        QAction *actionOf(QObject *object) {
            if (auto *menu = qobject_cast<QMenu *>(object)) return menu->menuAction();
            return qobject_cast<QAction *>(object);
        }
    }

    bool Simple() {
        return Configs::dataManager != nullptr && Configs::dataManager->settingsRepo->simple_mode;
    }

    bool IsAdvanced(const QObject *object) {
        if (tagged(object)) return true;
        auto *widget = qobject_cast<const QWidget *>(object);
        if (!widget) return false;
        for (const QWidget *parent = widget->parentWidget(); parent; parent = parent->parentWidget()) {
            if (tagged(parent)) return true;
        }
        auto *label = qobject_cast<const QLabel *>(widget);
        return label && tagged(rowField(label));
    }

    bool Allows(const QObject *object) {
        return !Simple() || !IsAdvanced(object);
    }

    void MarkAdvanced(QObject *object) {
        object->setProperty(kLevelProperty, QStringLiteral("advanced"));
    }

    void Unmark(QObject *object) {
        object->setProperty(kLevelProperty, QVariant());
    }

    void Apply(QWidget *root) {
        const bool simple = Simple();
        for (QObject *object : root->findChildren<QObject *>()) {
            if (!tagged(object)) continue;
            if (auto *action = actionOf(object)) {
                action->setVisible(wanted(action, action->isVisible()) && !simple);
            } else if (auto *widget = qobject_cast<QWidget *>(object)) {
                place(widget, wanted(widget, shown(widget)) && !simple);
            }
        }
    }

    void SetVisible(QWidget *widget, bool visible) {
        widget->setProperty(kWantedProperty, visible);
        place(widget, visible && !(Simple() && tagged(widget)));
    }

    void SetVisible(QAction *action, bool visible) {
        action->setProperty(kWantedProperty, visible);
        action->setVisible(visible && !(Simple() && (tagged(action) || tagged(action->menu()))));
    }
}
