---
slug: /ru/sql-reference/statements/create/table
sidebar_position: 36
sidebar_label: "Таблица"
---

# CREATE TABLE {#create-table-query}

Запрос `CREATE TABLE` может иметь несколько форм, которые используются в зависимости от контекста и решаемых задач.

По умолчанию таблицы создаются на текущем сервере. Распределенные DDL запросы создаются с помощью секции `ON CLUSTER`, которая [описана отдельно](../../../sql-reference/distributed-ddl.md).
## Варианты синтаксиса {#syntax-forms}
### С описанием структуры {#with-explicit-schema}

``` sql
CREATE TABLE [IF NOT EXISTS] [db.]table_name [ON CLUSTER cluster]
(
    name1 [type1] [NULL|NOT NULL] [DEFAULT|MATERIALIZED|EPHEMERAL|ALIAS expr1] [compression_codec] [TTL expr1],
    name2 [type2] [NULL|NOT NULL] [DEFAULT|MATERIALIZED|EPHEMERAL|ALIAS expr2] [compression_codec] [TTL expr2],
    ...
) ENGINE = engine
```

Создаёт таблицу с именем name в БД db или текущей БД, если db не указана, со структурой, указанной в скобках, и движком engine.
Структура таблицы представляет список описаний столбцов. Индексы, если поддерживаются движком, указываются в качестве параметров для движка таблицы.

Описание столбца, это `name type`, в простейшем случае. Пример: `RegionID UInt32`.
Также могут быть указаны выражения для значений по умолчанию - смотрите ниже.

При необходимости можно указать [первичный ключ](#primary-key) с одним или несколькими ключевыми выражениями.

### Со структурой, аналогичной другой таблице {#with-a-schema-similar-to-other-table}

``` sql
CREATE TABLE [IF NOT EXISTS] [db.]table_name AS [db2.]name2 [ENGINE = engine]
```

Создаёт таблицу с такой же структурой, как другая таблица. Можно указать другой движок для таблицы. Если движок не указан, то будет выбран такой же движок, как у таблицы `db2.name2`.

### Из табличной функции {#from-a-table-function}

``` sql
CREATE TABLE [IF NOT EXISTS] [db.]table_name AS table_function()
```
Создаёт таблицу с такой же структурой и данными, как результат соответствующей табличной функции. Созданная таблица будет работать так же, как и указанная табличная функция.

### Из запроса SELECT {#from-select-query}

``` sql
CREATE TABLE [IF NOT EXISTS] [db.]table_name[(name1 [type1], name2 [type2], ...)] ENGINE = engine AS SELECT ...
```

Создаёт таблицу со структурой, как результат запроса `SELECT`, с движком `engine`, и заполняет её данными из `SELECT`. Также вы можете явно задать описание столбцов.

Если таблица уже существует и указано `IF NOT EXISTS`, то запрос ничего не делает.

После секции `ENGINE` в запросе могут использоваться и другие секции в зависимости от движка. Подробную документацию по созданию таблиц смотрите в описаниях [движков таблиц](/engines/table-engines).

**Пример**

Запрос:

``` sql
CREATE TABLE t1 (x String) ENGINE = Memory AS SELECT 1;
SELECT x, toTypeName(x) FROM t1;
```

Результат:

```text
┌─x─┬─toTypeName(x)─┐
│ 1 │ String        │
└───┴───────────────┘
```

## Модификатор NULL или NOT NULL {#null-modifiers}

Модификатор `NULL` или `NOT NULL`, указанный после типа данных в определении столбца, позволяет или не позволяет типу данных быть [Nullable](/sql-reference/data-types/nullable).

Если тип не `Nullable` и указан модификатор `NULL`, то столбец будет иметь тип `Nullable`; если `NOT NULL`, то не `Nullable`. Например, `INT NULL` то же, что и `Nullable(INT)`. Если тип `Nullable` и указаны модификаторы `NULL` или `NOT NULL`, то будет вызвано исключение.

Смотрите также настройку [data_type_default_nullable](../../../operations/settings/settings.md#data_type_default_nullable).

## Значения по умолчанию {#create-default-values}

В описании столбца, может быть указано выражение для значения по умолчанию, одного из следующих видов:
`DEFAULT expr`, `MATERIALIZED expr`, `ALIAS expr`.
Пример: `URLDomain String DEFAULT domain(URL)`.

Если выражение для значения по умолчанию не указано, то в качестве значений по умолчанию будут использоваться нули для чисел, пустые строки для строк, пустые массивы для массивов, а также `0000-00-00` для дат и `0000-00-00 00:00:00` для дат с временем. NULL-ы не поддерживаются.

В случае, если указано выражение по умолчанию, то указание типа столбца не обязательно. При отсутствии явно указанного типа, будет использован тип выражения по умолчанию. Пример: `EventDate DEFAULT toDate(EventTime)` - для столбца EventDate будет использован тип Date.

При наличии явно указанного типа данных и выражения по умолчанию, это выражение будет приводиться к указанному типу с использованием функций приведения типа. Пример: `Hits UInt32 DEFAULT 0` - имеет такой же смысл, как `Hits UInt32 DEFAULT toUInt32(0)`.

В качестве выражения для умолчания, может быть указано произвольное выражение от констант и столбцов таблицы. При создании и изменении структуры таблицы, проверяется, что выражения не содержат циклов. При INSERT-е проверяется разрешимость выражений - что все столбцы, из которых их можно вычислить, переданы.

### DEFAULT {#default}

`DEFAULT expr`

Обычное значение по умолчанию. Если в запросе INSERT не указан соответствующий столбец, то он будет заполнен путём вычисления соответствующего выражения.

### MATERIALIZED {#materialized}

`MATERIALIZED expr`

Материализованное выражение. Такой столбец не может быть указан при INSERT, то есть, он всегда вычисляется.
При INSERT без указания списка столбцов, такие столбцы не рассматриваются.
Также этот столбец не подставляется при использовании звёздочки в запросе SELECT. Это необходимо, чтобы сохранить инвариант, что дамп, полученный путём `SELECT *`, можно вставить обратно в таблицу INSERT-ом без указания списка столбцов.

### EPHEMERAL {#ephemeral}

`EPHEMERAL [expr]`

Эфемерное выражение. Такой столбец не хранится в таблице и не может быть получен в запросе SELECT, но на него можно ссылаться в выражениях по умолчанию запроса CREATE. Если значение по умолчанию `expr` не указано, то тип колонки должен быть специфицирован.
INSERT без списка столбцов игнорирует этот столбец, таким образом сохраняется инвариант - т.е. дамп, полученный путём `SELECT *`, можно вставить обратно в таблицу INSERT-ом без указания списка столбцов.

### ALIAS {#alias}

`ALIAS expr`

Синоним. Такой столбец вообще не хранится в таблице.
Его значения не могут быть вставлены в таблицу, он не подставляется при использовании звёздочки в запросе SELECT.
Он может быть использован в SELECT-ах - в таком случае, во время разбора запроса, алиас раскрывается.

При добавлении новых столбцов с помощью запроса ALTER, старые данные для этих столбцов не записываются. Вместо этого, при чтении старых данных, для которых отсутствуют значения новых столбцов, выполняется вычисление выражений по умолчанию на лету. При этом, если выполнение выражения требует использования других столбцов, не указанных в запросе, то эти столбцы будут дополнительно прочитаны, но только для тех блоков данных, для которых это необходимо.

Если добавить в таблицу новый столбец, а через некоторое время изменить его выражение по умолчанию, то используемые значения для старых данных (для данных, где значения не хранились на диске) поменяются. Также заметим, что при выполнении фоновых слияний, данные для столбцов, отсутствующих в одном из сливаемых кусков, записываются в объединённый кусок.

Отсутствует возможность задать значения по умолчанию для элементов вложенных структур данных.

## Первичный ключ {#primary-key}

Вы можете определить [первичный ключ](../../../engines/table-engines/mergetree-family/mergetree.md#primary-keys-and-indexes-in-queries) при создании таблицы. Первичный ключ может быть указан двумя способами:

- в списке столбцов:

``` sql
CREATE TABLE db.table_name
(
    name1 type1, name2 type2, ...,
    PRIMARY KEY(expr1[, expr2,...])]
)
ENGINE = engine;
```

- вне списка столбцов:

``` sql
CREATE TABLE db.table_name
(
    name1 type1, name2 type2, ...
)
ENGINE = engine
PRIMARY KEY(expr1[, expr2,...]);
```

:::danger Предупреждение
Вы не можете сочетать оба способа в одном запросе.
:::

## Ограничения {#constraints}

Наряду с объявлением столбцов можно объявить ограничения на значения в столбцах таблицы:

``` sql
CREATE TABLE [IF NOT EXISTS] [db.]table_name [ON CLUSTER cluster]
(
    name1 [type1] [DEFAULT|MATERIALIZED|ALIAS expr1] [compression_codec] [TTL expr1],
    ...
    CONSTRAINT constraint_name_1 CHECK boolean_expr_1,
    ...
) ENGINE = engine
```

`boolean_expr_1` может быть любым булевым выражением, состоящим из операторов сравнения или функций. При наличии одного или нескольких ограничений в момент вставки данных выражения ограничений будут проверяться на истинность для каждой вставляемой строки данных. В случае, если в теле INSERT запроса придут некорректные данные — клиент получит исключение с описанием нарушенного ограничения.

Добавление большого числа ограничений может негативно повлиять на производительность `INSERT` запросов.

## Выражение для TTL {#vyrazhenie-dlia-ttl}

Определяет время хранения значений. Может быть указано только для таблиц семейства MergeTree. Подробнее смотрите в [TTL для столбцов и таблиц](../../../engines/table-engines/mergetree-family/mergetree.md#table_engine-mergetree-ttl).

## Кодеки сжатия столбцов {#codecs}

По умолчанию, ClickHouse применяет к столбцу метод сжатия, определённый в [конфигурации сервера](../../../operations/server-configuration-parameters/settings.md). Кроме этого, можно задать метод сжатия для каждого отдельного столбца в запросе `CREATE TABLE`.

``` sql
CREATE TABLE codec_example
(
    dt Date CODEC(ZSTD),
    ts DateTime CODEC(LZ4HC),
    float_value Float32 CODEC(NONE),
    double_value Float64 CODEC(LZ4HC(9)),
    value Float32 CODEC(Delta, ZSTD)
)
ENGINE = <Engine>
...
```

Если кодек `Default` задан для столбца, используется сжатие по умолчанию, которое может зависеть от различных настроек (и свойств данных) во время выполнения.
Пример: `value UInt64 CODEC(Default)` — то же самое, что не указать кодек.

Также можно подменить кодек столбца сжатием по умолчанию, определенным в config.xml:

``` sql
ALTER TABLE codec_example MODIFY COLUMN float_value CODEC(Default);
```

Кодеки можно последовательно комбинировать, например, `CODEC(Delta, Default)`.

:::danger Предупреждение
Нельзя распаковать базу данных ClickHouse с помощью сторонних утилит наподобие `lz4`. Необходимо использовать специальную утилиту [clickhouse-compressor](https://github.com/ClickHouse/ClickHouse/tree/master/programs/compressor).
:::

Сжатие поддерживается для следующих движков таблиц:

-   [MergeTree family](../../../engines/table-engines/mergetree-family/mergetree.md)
-   [Log family](../../../engines/table-engines/log-family/index.md)
-   [Set](../../../engines/table-engines/special/set.md)
-   [Join](../../../engines/table-engines/special/join.md)

ClickHouse поддерживает кодеки общего назначения и специализированные кодеки.

### Кодеки общего назначения {#create-query-common-purpose-codecs}

Кодеки:

-   `NONE` — без сжатия.
-   `LZ4` — [алгоритм сжатия без потерь](https://github.com/lz4/lz4) используемый по умолчанию. Применяет быстрое сжатие LZ4.
-   `LZ4HC[(level)]` — алгоритм LZ4 HC (high compression) с настраиваемым уровнем сжатия. Уровень по умолчанию — 9. Настройка `level <= 0` устанавливает уровень сжания по умолчанию. Возможные уровни сжатия: \[1, 12\]. Рекомендуемый диапазон уровней: \[4, 9\].
-   `ZSTD[(level)]` — [алгоритм сжатия ZSTD](https://en.wikipedia.org/wiki/Zstandard) с настраиваемым уровнем сжатия `level`. Возможные уровни сжатия: \[1, 22\]. Уровень сжатия по умолчанию: 1.

Высокие уровни сжатия полезны для ассимметричных сценариев, подобных «один раз сжал, много раз распаковал». Они подразумевают лучшее сжатие, но большее использование CPU.

### Специализированные кодеки {#create-query-specialized-codecs}

Эти кодеки разработаны для того, чтобы, используя особенности данных сделать сжатие более эффективным. Некоторые из этих кодеков не сжимают данные самостоятельно. Они готовят данные для кодеков общего назначения, которые сжимают подготовленные данные эффективнее, чем неподготовленные.

Специализированные кодеки:

-   `Delta(delta_bytes)` — Метод, в котором исходные значения заменяются разностью двух соседних значений, за исключением первого значения, которое остаётся неизменным. Для хранения разниц используется до `delta_bytes`, т.е. `delta_bytes` — это максимальный размер исходных данных. Возможные значения `delta_bytes`: 1, 2, 4, 8. Значение по умолчанию для `delta_bytes` равно `sizeof(type)`, если результат 1, 2, 4, or 8. Во всех других случаях — 1.
-   `DoubleDelta` — Вычисляет разницу от разниц и сохраняет её в компактном бинарном виде. Оптимальная степень сжатия достигается для монотонных последовательностей с постоянным шагом, наподобие временных рядов. Можно использовать с любым типом данных фиксированного размера. Реализует алгоритм, используемый в TSDB Gorilla, поддерживает 64-битные типы данных. Использует 1 дополнительный бит для 32-байтовых значений: 5-битные префиксы вместо 4-битных префиксов. Подробнее читайте в разделе «Compressing Time Stamps» документа [Gorilla: A Fast, Scalable, In-Memory Time Series Database](http://www.vldb.org/pvldb/vol8/p1816-teller.pdf).
-   `GCD` - Вычисляет НОД всех чисел, а затем делит их на него. Этот кодек предназначен для подготовки данных и не подходит для использования без дополнительного кодека. GCD-кодек может использоваться с Integer, Decimal и DateTime. Хорошим вариантом использования было бы хранение временных меток или денежных значений с высокой точностью.
-   `Gorilla` — Вычисляет XOR между текущим и предыдущим значением и записывает результат в компактной бинарной форме. Эффективно сохраняет ряды медленно изменяющихся чисел с плавающей запятой, поскольку наилучший коэффициент сжатия достигается, если соседние значения одинаковые. Реализует алгоритм, используемый в TSDB Gorilla, адаптируя его для работы с 64-битными значениями. Подробнее читайте в разделе «Compressing Values» документа [Gorilla: A Fast, Scalable, In-Memory Time Series Database](http://www.vldb.org/pvldb/vol8/p1816-teller.pdf).
-   `T64` — Метод сжатия который обрезает неиспользуемые старшие биты целочисленных значений (включая `Enum`, `Date` и `DateTime`). На каждом шаге алгоритма, кодек помещает блок из 64 значений в матрицу 64✕64, транспонирует её, обрезает неиспользуемые биты, а то, что осталось возвращает в виде последовательности. Неиспользуемые биты, это биты, которые не изменяются от минимального к максимальному на всём диапазоне значений куска данных.

Кодеки `DoubleDelta` и `Gorilla` используются в TSDB Gorilla как компоненты алгоритма сжатия. Подход Gorilla эффективен в сценариях, когда данные представляют собой медленно изменяющиеся во времени величины. Метки времени эффективно сжимаются кодеком `DoubleDelta`, а значения кодеком `Gorilla`. Например, чтобы создать эффективно хранящуюся таблицу, используйте следующую конфигурацию:

``` sql
CREATE TABLE codec_example
(
    timestamp DateTime CODEC(DoubleDelta),
    slow_values Float32 CODEC(Gorilla)
)
ENGINE = MergeTree()
```

### Кодеки шифрования {#create-query-encryption-codecs}

Эти кодеки не сжимают данные, вместо этого они зашифровывают данные на диске. Воспользоваться кодеками можно, только когда ключ шифрования задан параметрами [шифрования](/operations/server-configuration-parameters/settings#encryption). Обратите внимание: ставить кодеки шифрования имеет смысл в самый конец цепочки кодеков, потому что зашифрованные данные, как правило, нельзя сжать релевантным образом.

Кодеки шифрования:

-   `CODEC('AES-128-GCM-SIV')` — Зашифровывает данные с помощью AES-128 в режиме [RFC 8452](https://tools.ietf.org/html/rfc8452) GCM-SIV.
-   `CODEC('AES-256-GCM-SIV')` — Зашифровывает данные с помощью AES-256 в режиме GCM-SIV.

Эти кодеки используют фиксированный одноразовый ключ шифрования. Таким образом, это детерминированное шифрование. Оно совместимо с поддерживающими дедупликацию движками, в частности, [ReplicatedMergeTree](../../../engines/table-engines/mergetree-family/replication.md). Однако у шифрования имеется недостаток: если дважды зашифровать один и тот же блок данных, текст на выходе получится одинаковым, и злоумышленник, у которого есть доступ к диску, заметит эту эквивалентность (при этом доступа к содержимому он не получит).

:::note Внимание
Большинство движков, включая семейство `MergeTree`, создают на диске индексные файлы, не применяя кодеки. А значит, в том случае, если зашифрованный столбец индексирован, на диске отобразится незашифрованный текст.
:::
:::note Внимание
Если вы выполняете запрос SELECT с упоминанием конкретного значения в зашифрованном столбце (например, при использовании секции WHERE), это значение может появиться в [system.query_log](../../../operations/system-tables/query_log.md). Рекомендуем отключить логирование.
:::
**Пример**

```sql
CREATE TABLE mytable
(
    x String Codec(AES_128_GCM_SIV)
)
ENGINE = MergeTree ORDER BY x;
```

:::note Примечание
Если необходимо применить сжатие, это нужно явно прописать в запросе. Без этого будет выполнено только шифрование данных.
:::

**Пример**

```sql
CREATE TABLE mytable
(
    x String Codec(Delta, LZ4, AES_128_GCM_SIV)
)
ENGINE = MergeTree ORDER BY x;
```

## Временные таблицы {#temporary-tables}

ClickHouse поддерживает временные таблицы со следующими характеристиками:

-   Временные таблицы исчезают после завершения сессии, в том числе при обрыве соединения.
-   Временная таблица использует движок таблиц Memory когда движок не указан и она может использовать любой движок таблиц за исключением движков Replicated и `KeeperMap`.
-   Невозможно указать базу данных для временной таблицы. Она создается вне баз данных.
-   Невозможно создать временную таблицу распределённым DDL запросом на всех серверах кластера (с опцией `ON CLUSTER`): такая таблица существует только в рамках существующей сессии.
-   Если временная таблица имеет то же имя, что и некоторая другая, то, при упоминании в запросе без указания БД, будет использована временная таблица.
-   При распределённой обработке запроса, используемые в запросе временные таблицы, передаются на удалённые серверы.

Чтобы создать временную таблицу, используйте следующий синтаксис:

``` sql
CREATE TEMPORARY TABLE [IF NOT EXISTS] table_name
(
    name1 [type1] [DEFAULT|MATERIALIZED|ALIAS expr1],
    name2 [type2] [DEFAULT|MATERIALIZED|ALIAS expr2],
    ...
) [ENGINE = engine]
```

В большинстве случаев, временные таблицы создаются не вручную, а при использовании внешних данных для запроса, или при распределённом `(GLOBAL) IN`. Подробнее см. соответствующие разделы

Вместо временных можно использовать обычные таблицы с [ENGINE = Memory](../../../engines/table-engines/special/memory.md).

## REPLACE TABLE {#replace-table-query}

Запрос `REPLACE` позволяет частично изменить таблицу (структуру или данные).

:::note Примечание
Такие запросы поддерживаются только движком БД [Atomic](../../../engines/database-engines/atomic.md).
:::

Чтобы удалить часть данных из таблицы, вы можете создать новую таблицу, добавить в нее данные из старой таблицы, которые вы хотите оставить (отобрав их с помощью запроса `SELECT`), затем удалить старую таблицу и переименовать новую таблицу так как старую:

```sql
CREATE TABLE myNewTable AS myOldTable;
INSERT INTO myNewTable SELECT * FROM myOldTable WHERE CounterID <12345;
DROP TABLE myOldTable;
RENAME TABLE myNewTable TO myOldTable;
```

Вместо перечисленных выше операций можно использовать один запрос:

```sql
REPLACE TABLE myOldTable SELECT * FROM myOldTable WHERE CounterID <12345;
```

### Синтаксис

```sql
{CREATE [OR REPLACE]|REPLACE} TABLE [db.]table_name
```

Для данного запроса можно использовать любые варианты синтаксиса запроса `CREATE`. Запрос `REPLACE` для несуществующей таблицы вызовет ошибку.

### Примеры:

Рассмотрим таблицу:

```sql
CREATE DATABASE base ENGINE = Atomic;
CREATE OR REPLACE TABLE base.t1 (n UInt64, s String) ENGINE = MergeTree ORDER BY n;
INSERT INTO base.t1 VALUES (1, 'test');
SELECT * FROM base.t1;
```

```text
┌─n─┬─s────┐
│ 1 │ test │
└───┴──────┘
```

Используем запрос `REPLACE` для удаления всех данных:

```sql
CREATE OR REPLACE TABLE base.t1 (n UInt64, s Nullable(String)) ENGINE = MergeTree ORDER BY n;
INSERT INTO base.t1 VALUES (2, null);
SELECT * FROM base.t1;
```

```text
┌─n─┬─s──┐
│ 2 │ \N │
└───┴────┘
```

Используем запрос `REPLACE` для изменения структуры таблицы:

```sql
REPLACE TABLE base.t1 (n UInt64) ENGINE = MergeTree ORDER BY n;
INSERT INTO base.t1 VALUES (3);
SELECT * FROM base.t1;
```

```text
┌─n─┐
│ 3 │
└───┘
```

## Секция COMMENT {#comment-table}

Вы можете добавить комментарий к таблице при ее создании.

:::note Примечание
Комментарий поддерживается для всех движков таблиц, кроме [Kafka](../../../engines/table-engines/integrations/kafka.md), [RabbitMQ](../../../engines/table-engines/integrations/rabbitmq.md) и [EmbeddedRocksDB](../../../engines/table-engines/integrations/embedded-rocksdb.md).
:::
**Синтаксис**

``` sql
CREATE TABLE db.table_name
(
    name1 type1, name2 type2, ...
)
ENGINE = engine
COMMENT 'Comment'
```

**Пример**

Запрос:

``` sql
CREATE TABLE t1 (x String) ENGINE = Memory COMMENT 'The temporary table';
SELECT name, comment FROM system.tables WHERE name = 't1';
```

Результат:

```text
┌─name─┬─comment─────────────┐
│ t1   │ The temporary table │
└──────┴─────────────────────┘
```
