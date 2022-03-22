#ifndef GLOBALS_H
#define GLOBALS_H

#include <type_traits>
#include <QString>
#include <QMetaEnum>

namespace SYS
{
    enum class enum_status { IN_QUEUE, RUNNING, DONE, CANCELLED };

    enum class enum_table_headers { ID, DESCRIPTION, STATUS, RESULT, DURATION};

    /* возвращает приведенное к родному типу значение класса enum'a */
    template<typename E>
    constexpr std::underlying_type_t<E>
    toUType(E enumerator) noexcept {
        return static_cast<std::underlying_type_t<E>>(enumerator);
    }
}


#endif // GLOBALS_H
