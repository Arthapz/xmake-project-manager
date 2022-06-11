// Copyright (C) 2022 Arthur LAURENT <arthur.laurent4@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level of this distribution

#pragma once

namespace XMakeProjectManager::Internal {
    ////////////////////////////////////////////////////
    ////////////////////////////////////////////////////
    inline CancellableOption::CancellableOption(const BuildOption &option, bool locked)
        : m_locked { locked } {
        m_saved_value   = std::make_unique<BuildOption>(option);
        m_current_value = std::make_unique<BuildOption>(option);
    }

    ////////////////////////////////////////////////////
    ////////////////////////////////////////////////////
    inline bool CancellableOption::locked() const noexcept { return m_locked; }

    ////////////////////////////////////////////////////
    ////////////////////////////////////////////////////
    inline bool CancellableOption::changed() const noexcept { return m_changed; }

    ////////////////////////////////////////////////////
    ////////////////////////////////////////////////////
    inline void CancellableOption::apply() {
        if (m_changed) {
            m_saved_value = std::make_unique<BuildOption>(*m_current_value);
            m_changed     = false;
        }
    }

    ////////////////////////////////////////////////////
    ////////////////////////////////////////////////////
    inline void CancellableOption::cancel() {
        if (m_changed) {
            m_current_value = std::make_unique<BuildOption>(*m_saved_value);
            m_changed       = false;
        }
    }

    ////////////////////////////////////////////////////
    ////////////////////////////////////////////////////
    inline const QString &CancellableOption::name() const noexcept { return m_current_value->name; }

    ////////////////////////////////////////////////////
    ////////////////////////////////////////////////////
    inline const QString &CancellableOption::description() const noexcept {
        return m_current_value->description;
    }

    ////////////////////////////////////////////////////
    ////////////////////////////////////////////////////
    inline const QString &CancellableOption::savedValue() const noexcept {
        return m_saved_value->value;
    }

    ////////////////////////////////////////////////////
    ////////////////////////////////////////////////////
    inline const QStringList &CancellableOption::values() const noexcept {
        return m_current_value->values;
    }

    ////////////////////////////////////////////////////
    ////////////////////////////////////////////////////
    inline QString CancellableOption::xmakeArg() const noexcept {
        return m_current_value->xmakeArg();
    }

    ////////////////////////////////////////////////////
    ////////////////////////////////////////////////////
    inline void CancellableOption::setValue(const QString &value) {
        if (!m_locked) {
            m_current_value->value = value;
            m_changed              = m_current_value->value != m_saved_value->value;
        }
    }

    ////////////////////////////////////////////////////
    ////////////////////////////////////////////////////
    inline const QString &CancellableOption::value() const noexcept {
        return m_current_value->value;
    }

    ////////////////////////////////////////////////////
    ////////////////////////////////////////////////////
    inline BuildOptionTreeItem::BuildOptionTreeItem(CancellableOption &option) noexcept
        : m_option { &option } {}

    ////////////////////////////////////////////////////
    ////////////////////////////////////////////////////
    inline auto BuildOptionTreeItem::data(int column, int role) const noexcept -> QVariant {
        QTC_ASSERT(column >= 0 && column < 2, return {});
        QTC_ASSERT(m_option, return {});

        if (column == 0) {
            switch (role) {
                case Qt::DisplayRole: return m_option->name();
                case Qt::ToolTipRole: return toolTip();
                case Qt::FontRole:
                    auto font = QFont {};
                    font.setBold(m_option->changed());

                    return font;
            }
        } else if (column == 1) {
            switch (role) {
                case Qt::DisplayRole: [[fallthrough]];
                case Qt::EditRole: return m_option->value();
                case Qt::UserRole: return m_option->locked();
                case Qt::UserRole + 1: return m_option->values();
                case Qt::ToolTipRole:
                    if (m_option->changed())
                        return QStringLiteral("%1<br>Initial value was <b>%2</b>")
                            .arg(toolTip())
                            .arg(m_option->savedValue());
                    else
                        return toolTip();
                case Qt::FontRole:
                    auto font = QFont {};
                    font.setBold(m_option->changed());

                    return font;
            }
        }

        return {};
    }

    ////////////////////////////////////////////////////
    ////////////////////////////////////////////////////
    inline auto BuildOptionTreeItem::setData(int column,
                                             const QVariant &data,
                                             [[maybe_unused]] int role) noexcept -> bool {
        QTC_ASSERT(column == 1, return false);

        m_option->setValue(data.toString());

        return true;
    }

    ////////////////////////////////////////////////////
    ////////////////////////////////////////////////////
    inline auto BuildOptionTreeItem::flags(int column) const noexcept -> Qt::ItemFlags {
        QTC_ASSERT(column >= 0 && column < 2, return Qt::NoItemFlags);

        if (column == 0) return Qt::ItemIsEnabled | Qt::ItemIsSelectable;

        return Qt::ItemIsEnabled | Qt::ItemIsSelectable | Qt::ItemIsEditable;
    }

    ////////////////////////////////////////////////////
    ////////////////////////////////////////////////////
    inline auto BuildOptionTreeItem::toolTip() const noexcept -> const QString & {
        return m_option->description();
    }
} // namespace XMakeProjectManager::Internal
