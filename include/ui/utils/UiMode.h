#pragma once

class QAction;
class QObject;
class QWidget;

// Simple mode hides what the forms tag with uiLevel="advanced"; settings keep their values and still apply.
namespace UiMode {
    bool Simple();

    // object, a widget that holds it, or the field its label names is tagged; the current mode plays no part.
    bool IsAdvanced(const QObject *object);
    bool Allows(const QObject *object);

    // The tag for items built in code; Unmark keeps an advanced switch in view while its state greys out a shown field.
    void MarkAdvanced(QObject *object);
    void Unmark(QObject *object);

    // Shows each tagged tab, form row, menu, action and widget under root as its owner last asked, unless Simple mode hides it.
    void Apply(QWidget *root);

    // For code that toggles a tagged item's visibility: a plain setVisible() would undo Simple mode.
    void SetVisible(QWidget *widget, bool visible);
    void SetVisible(QAction *action, bool visible);
}
