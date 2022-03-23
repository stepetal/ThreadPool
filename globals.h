#ifndef GLOBALS_H
#define GLOBALS_H

#include <type_traits>
#include <QString>
#include <QMetaEnum>

#define SILENT_MODE 1

namespace SYS
{
    enum class enum_status { IN_QUEUE, RUNNING, DONE, CANCELLED };

    enum class enum_table_headers { ID, DESCRIPTION, STATUS, RESULT, DURATION};

    enum class table_roles {roleId = Qt::UserRole, STATUS_ROLE };

    /* возвращает приведенное к родному типу значение класса enum'a */
    template<typename E>
    constexpr std::underlying_type_t<E>
    toUType(E enumerator) noexcept {
        return static_cast<std::underlying_type_t<E>>(enumerator);
    }
}


#endif // GLOBALS_H
