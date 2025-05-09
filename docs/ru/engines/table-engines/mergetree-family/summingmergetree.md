---
slug: /ru/engines/table-engines/mergetree-family/summingmergetree
sidebar_position: 34
sidebar_label: SummingMergeTree
---

# SummingMergeTree {#summingmergetree}

Движок наследует функциональность [MergeTree](mergetree.md#table_engines-mergetree). Отличие заключается в том, что для таблиц `SummingMergeTree` при слиянии кусков данных ClickHouse все строки с одинаковым первичным ключом (точнее, с одинаковым [ключом сортировки](mergetree.md)) заменяет на одну, которая хранит только суммы значений из столбцов с цифровым типом данных. Если ключ сортировки подобран таким образом, что одному значению ключа соответствует много строк, это значительно уменьшает объём хранения и ускоряет последующую выборку данных.

Мы рекомендуем использовать движок в паре с `MergeTree`. В `MergeTree` храните полные данные, а `SummingMergeTree` используйте для хранения агрегированных данных, например, при подготовке отчетов. Такой подход позволит не утратить ценные данные из-за неправильно выбранного первичного ключа.

## Создание таблицы {#sozdanie-tablitsy}

``` sql
CREATE TABLE [IF NOT EXISTS] [db.]table_name [ON CLUSTER cluster]
(
    name1 [type1] [DEFAULT|MATERIALIZED|ALIAS expr1],
    name2 [type2] [DEFAULT|MATERIALIZED|ALIAS expr2],
    ...
) ENGINE = SummingMergeTree([columns])
[PARTITION BY expr]
[ORDER BY expr]
[SAMPLE BY expr]
[SETTINGS name=value, ...]
```

Описание параметров запроса смотрите в [описании запроса](../../../engines/table-engines/mergetree-family/summingmergetree.md).

**Параметры SummingMergeTree**

-   `columns` — кортеж с именами столбцов, в которых будут суммироваться данные. Необязательный параметр.
    Столбцы должны иметь числовой тип и не должны входить в первичный ключ.

        Если `columns` не задан, то ClickHouse суммирует значения во всех столбцах с числовым типом данных, не входящих в первичный ключ.

**Секции запроса**

При создании таблицы `SummingMergeTree` используются те же [секции](mergetree.md) запроса, что и при создании таблицы `MergeTree`.

<details markdown="1">

<summary>Устаревший способ создания таблицы</summary>

:::note Важно
Не используйте этот способ в новых проектах и по возможности переведите старые проекты на способ описанный выше.
:::

``` sql
CREATE TABLE [IF NOT EXISTS] [db.]table_name [ON CLUSTER cluster]
(
    name1 [type1] [DEFAULT|MATERIALIZED|ALIAS expr1],
    name2 [type2] [DEFAULT|MATERIALIZED|ALIAS expr2],
    ...
) ENGINE [=] SummingMergeTree(date-column [, sampling_expression], (primary, key), index_granularity, [columns])
```

Все параметры, кроме `columns` имеют то же значение, что в и `MergeTree`.

-   `columns` — кортеж с именами столбцов для суммирования данных. Необязательный параметр. Описание смотрите выше по тексту.

</details>

## Пример использования {#primer-ispolzovaniia}

Рассмотрим следующую таблицу:

``` sql
CREATE TABLE summtt
(
    key UInt32,
    value UInt32
)
ENGINE = SummingMergeTree()
ORDER BY key
```

Добавим в неё данные:

``` sql
INSERT INTO summtt Values(1,1),(1,2),(2,1)
```

ClickHouse может не полностью просуммировать все строки ([смотрите ниже по тексту](#obrabotka-dannykh)), поэтому при запросе мы используем агрегатную функцию `sum` и секцию `GROUP BY`.

``` sql
SELECT key, sum(value) FROM summtt GROUP BY key
```

``` text
┌─key─┬─sum(value)─┐
│   2 │          1 │
│   1 │          3 │
└─────┴────────────┘
```

## Обработка данных {#obrabotka-dannykh}

При вставке данных в таблицу они сохраняются как есть. Периодически ClickHouse выполняет слияние вставленных кусков данных и именно в этот момент производится суммирование и замена многих строк с одинаковым первичным ключом на одну для каждого результирующего куска данных.

ClickHouse может слить куски данных таким образом, что не все строки с одинаковым первичным ключом окажутся в одном финальном куске, т.е. суммирование будет не полным. Поэтому, при выборке данных (`SELECT`) необходимо использовать агрегатную функцию [sum()](../../../engines/table-engines/mergetree-family/summingmergetree.md#agg_function-sum) и секцию `GROUP BY` как описано в примере выше.

### Общие правила суммирования {#obshchie-pravila-summirovaniia}

Суммируются значения в столбцах с числовым типом данных. Набор столбцов определяется параметром `columns`.

Если значения во всех столбцах для суммирования оказались нулевыми, то строчка удаляется.

Для столбцов, не входящих в первичный ключ и не суммирующихся, выбирается произвольное значение из имеющихся.

Значения для столбцов, входящих в первичный ключ, не суммируются.

### Суммирование в столбцах AggregateFunction {#summirovanie-v-stolbtsakh-aggregatefunction}

Для столбцов типа [AggregateFunction](../../../sql-reference/data-types/aggregatefunction.md#data-type-aggregatefunction) ClickHouse выполняет агрегацию согласно заданной функции, повторяя поведение движка [AggregatingMergeTree](aggregatingmergetree.md).

### Вложенные структуры {#vlozhennye-struktury}

Таблица может иметь вложенные структуры данных, которые обрабатываются особым образом.

Если название вложенной таблицы заканчивается на `Map` и она содержит не менее двух столбцов, удовлетворяющих критериям:

-   первый столбец - числовой `(*Int*, Date, DateTime)` или строковый `(String, FixedString)`, назовем его условно `key`,
-   остальные столбцы - арифметические `(*Int*, Float32/64)`, условно `(values...)`,

то вложенная таблица воспринимается как отображение `key => (values...)` и при слиянии её строк выполняется слияние элементов двух множеств по `key` со сложением соответствующих `(values...)`.

Примеры:

``` text
[(1, 100)] + [(2, 150)] -> [(1, 100), (2, 150)]
[(1, 100)] + [(1, 150)] -> [(1, 250)]
[(1, 100)] + [(1, 150), (2, 150)] -> [(1, 250), (2, 150)]
[(1, 100), (2, 150)] + [(1, -100)] -> [(2, 150)]
```

При запросе данных используйте функцию [sumMap(key, value)](/sql-reference/aggregate-functions/reference/summap) для агрегации `Map`.

Для вложенной структуры данных не нужно указывать её столбцы в кортеже столбцов для суммирования.
