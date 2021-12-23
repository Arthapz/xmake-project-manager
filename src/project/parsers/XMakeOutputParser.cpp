#include "XMakeOutputParser.hpp"

#include <utils/fileutils.h>

#include <projectexplorer/taskhub.h>

namespace XMakeProjectManager::Internal {
    ////////////////////////////////////////////////////
    ////////////////////////////////////////////////////
    XMakeOutputParser::XMakeOutputParser() = default;

    ////////////////////////////////////////////////////
    ////////////////////////////////////////////////////
    XMakeOutputParser::~XMakeOutputParser() = default;

    ////////////////////////////////////////////////////
    ////////////////////////////////////////////////////
    auto XMakeOutputParser::handleLine(const QString &line, Utils::OutputFormat type) -> Result {
        if (type != Utils::OutputFormat::StdOutFormat)
            return ProjectExplorer::OutputTaskParser::Status::NotHandled;

        if (m_remaining_lines > 0) {
            pushLine(line);
            return ProjectExplorer::OutputTaskParser::Status::Done;
        }

        auto result = processErrors(line);
        if (result.status == ProjectExplorer::OutputTaskParser::Status::Done) return result;

        return ProjectExplorer::OutputTaskParser::Status::NotHandled;
    }

    ////////////////////////////////////////////////////
    ////////////////////////////////////////////////////
    auto XMakeOutputParser::readStdo(const QByteArray &data) -> void {
        const auto lines = QString::fromLocal8Bit(data).split('\n');

        for (const auto &line : lines) handleLine(line, Utils::OutputFormat::StdOutFormat);
    }

    ////////////////////////////////////////////////////
    ////////////////////////////////////////////////////
    auto XMakeOutputParser::setSourceDirectory(const Utils::FilePath &source_dir) -> void {
        Q_EMIT new_search_dir_found(source_dir);
    }

    ////////////////////////////////////////////////////
    ////////////////////////////////////////////////////
    auto XMakeOutputParser::pushLine(const QString &line) -> void {
        m_remaining_lines--;

        m_pending_lines.push_back(line);

        if (m_remaining_lines == 0) {
            addTask(ProjectExplorer::Task::TaskType::Error, m_pending_lines.join('\n'));
            m_pending_lines.clear();
        }
    }

    ////////////////////////////////////////////////////
    ////////////////////////////////////////////////////
    auto XMakeOutputParser::processErrors(const QString& line) -> Result
    {
        auto options_errors = m_options_errors_regex.match(line);
        if (options_errors.hasMatch()) {
            addTask(ProjectExplorer::Task::TaskType::Error, line);
            return ProjectExplorer::OutputTaskParser::Status::Done;
        }

        auto located_errors = m_error_file_location_regex.match(line);
        if (located_errors.hasMatch()) {
            auto link_specs = addTask(ProjectExplorer::Task::TaskType::Error,
                                      line,
                                      located_errors,
                                      QStringLiteral("error"),
                                      1,
                                      2,
                                      3);
            return { ProjectExplorer::OutputTaskParser::Status::Done, link_specs };
        }

        return ProjectExplorer::OutputTaskParser::Status::NotHandled;
    }

    ////////////////////////////////////////////////////
    ////////////////////////////////////////////////////
    auto XMakeOutputParser::addTask(ProjectExplorer::Task::TaskType type, QStringView line)
        -> void {
        auto task = ProjectExplorer::BuildSystemTask {
            type,
            QString { QStringLiteral("XMake configuration: %1") }.arg(line)
        };
        addTask(std::move(task));
    }

    ////////////////////////////////////////////////////
    ////////////////////////////////////////////////////
    auto XMakeOutputParser::addTask(ProjectExplorer::Task task) -> void {
        ProjectExplorer::TaskHub::addTask(task);
    }

    ////////////////////////////////////////////////////
    ////////////////////////////////////////////////////
    auto XMakeOutputParser::addTask(ProjectExplorer::Task::TaskType type,
                                    QStringView line,
                                    const QRegularExpressionMatch &match,
                                    QStringView output_type,
                                    int file_cap_index,
                                    int line_number_cap_index,
                                    int error_cap_index) -> LinkSpecs {
        auto link_specs = LinkSpecs {};

        auto file_name =
            absoluteFilePath(Utils::FilePath::fromString(match.captured(file_cap_index)));
        auto task = ProjectExplorer::BuildSystemTask {
            type,
            QString { QStringLiteral("XMake configuration %1: %2") }.arg(output_type,
                                                                         match.captured(
                                                                             error_cap_index)),
            file_name,
            match.captured(line_number_cap_index).toInt()
        };

        addLinkSpecForAbsoluteFilePath(link_specs, task.file, task.line, match, 1);
        addTask(std::move(task));

        return link_specs;
    }
} // namespace XMakeProjectManager::Internal
