#ifndef CondCore_ConditionDatabase_DbCore_h
#define CondCore_ConditionDatabase_DbCore_h
//
// Package:     CondDB
// Class  :     DbCore
//
/**\class DbCore DbCore.h CondCore/CondDB/interface/DbCore.h
   Description: an interface wrapper for CORAL.  
*/
//
// Author:      Miguel Ojeda, Giacomo Govi
// Created:     May 2013
//
//

#include "CondCore/CondDB/interface/Exception.h"
#include "CondCore/CondDB/interface/Binary.h"
#include "CondCore/CondDB/interface/Time.h"
#include "CondCore/CondDB/interface/Types.h"
// coral includes
#include "CoralBase/AttributeList.h"
#include "CoralBase/Attribute.h"
#include "CoralBase/AttributeSpecification.h"
#include "CoralBase/Blob.h"
#include "CoralBase/TimeStamp.h"
#include "RelationalAccess/ICursor.h"
#include "RelationalAccess/ISchema.h"
#include "RelationalAccess/ISessionProxy.h"
#include "RelationalAccess/IQuery.h"
#include "RelationalAccess/TableDescription.h"
#include "RelationalAccess/ITable.h"
#include "RelationalAccess/IColumn.h"
#include "RelationalAccess/ITableDataEditor.h"
#include "RelationalAccess/IBulkOperation.h"
#include "RelationalAccess/SchemaException.h"
//
#include <tuple>
#include <cstring>
#include <set>
#include <map>
#include <memory>
#include <string_view>

//
#include <boost/date_time/posix_time/posix_time.hpp>

// macros for the schema definition

namespace condcore_detail {
  consteval int string_size(char const* iSize) { return std::string_view(iSize).size(); }

  template <int N, int M>
  consteval std::array<char, N + M + 2> makeFullName(const char* n, const char* m) {
    if (std::string_view(n).size() != N) {
      throw "makeFullName first argument incorrect size";
    }
    if (std::string_view(m).size() != M) {
      throw "makeFullName second argument incorrect size";
    }
    std::array<char, N + M + 2> ret = {};
    for (int i = 0; i < N; ++i) {
      ret[i] = n[i];
    }
    ret[N] = '.';
    for (int i = 0; i < M; ++i) {
      ret[i + N + 1] = m[i];
    }
    return ret;
  }

  consteval void test_makeFullName() {
    constexpr auto v = makeFullName<2, 3>("ab", "cde");
    static_assert(v.size() == 2 + 3 + 1 + 1);
    static_assert('a' == v[0]);
    static_assert('b' == v[1]);
    static_assert('.' == v[2]);
    static_assert('c' == v[3]);
    static_assert('d' == v[4]);
    static_assert('e' == v[5]);
    static_assert(0 == v[6]);
  }

  template <int N>
  consteval std::array<char, N + 5 + 1> addMin(std::string_view n) {
    if (n.size() != N) {
      throw "addMin argument wrong size";
    }
    std::array<char, N + 5 + 1> ret = {};
    ret[0] = 'M';
    ret[1] = 'I';
    ret[2] = 'N';
    ret[3] = '(';
    for (int i = 0; i < N; ++i) {
      ret[4 + i] = n[i];
    }
    ret[N + 4] = ')';
    return ret;
  }

  consteval void test_addMin() {
    constexpr std::string_view foo("Foo");
    constexpr auto addMin_ = addMin<3>(foo);
    static_assert(addMin_.size() == 9);
    static_assert(addMin_[0] == 'M');
    static_assert(addMin_[1] == 'I');
    static_assert(addMin_[2] == 'N');
    static_assert(addMin_[3] == '(');
    static_assert(addMin_[4] == 'F');
    static_assert(addMin_[5] == 'o');
    static_assert(addMin_[6] == 'o');
    static_assert(addMin_[7] == ')');
    static_assert(addMin_[8] == 0);
  }

  template <int N>
  consteval std::array<char, N + 5 + 1> addMax(std::string_view n) {
    if (n.size() != N) {
      throw "addMax argument wrong size";
    }
    std::array<char, N + 5 + 1> ret = {};
    ret[0] = 'M';
    ret[1] = 'A';
    ret[2] = 'X';
    ret[3] = '(';
    for (int i = 0; i < N; ++i) {
      ret[4 + i] = n[i];
    }
    ret[N + 4] = ')';
    return ret;
  }

  consteval void test_addMax() {
    constexpr std::string_view foo("Foo");
    constexpr auto addMin_ = addMax<3>(foo);
    static_assert(addMin_.size() == 9);
    static_assert(addMin_[0] == 'M');
    static_assert(addMin_[1] == 'A');
    static_assert(addMin_[2] == 'X');
    static_assert(addMin_[3] == '(');
    static_assert(addMin_[4] == 'F');
    static_assert(addMin_[5] == 'o');
    static_assert(addMin_[6] == 'o');
    static_assert(addMin_[7] == ')');
    static_assert(addMin_[8] == 0);
  }

}  // namespace condcore_detail

// table definition
#define conddb_table(NAME)                      \
  namespace NAME {                              \
    static constexpr char const* tname = #NAME; \
  }                                             \
  namespace NAME

// implementation for the column definition:

// case with 3 params
#define FIXSIZE_COLUMN(NAME, TYPE, SIZE)                                                                              \
  struct NAME {                                                                                                       \
    static constexpr char const* name = #NAME;                                                                        \
    typedef TYPE type;                                                                                                \
    static constexpr size_t size = SIZE;                                                                              \
    static constexpr std::string_view tableName() { return std::string_view(tname); }                                 \
    static constexpr auto fullyQualifiedName_ =                                                                       \
        condcore_detail::makeFullName<condcore_detail::string_size(tname), condcore_detail::string_size(name)>(tname, \
                                                                                                               name); \
    static constexpr std::string_view fullyQualifiedName() { return std::string_view(fullyQualifiedName_.data()); }   \
  };

// case with 2 params
#define VARSIZE_COLUMN(NAME, TYPE) FIXSIZE_COLUMN(NAME, TYPE, 0)

// trick to select the case
#define GET_4TH_ARG(arg1, arg2, arg3, arg4, ...) arg4
#define WRONG_PAR_NUMBER_ERROR(...) static_assert(false, "\"column\" macro accepts exactly 2 or 3 parameters")
#define SELECT_COLUMN_MACRO(...) GET_4TH_ARG(__VA_ARGS__, FIXSIZE_COLUMN, VARSIZE_COLUMN, WRONG_PAR_NUMBER_ERROR)

// the final column definition macro
#define conddb_column(...) SELECT_COLUMN_MACRO(__VA_ARGS__)(__VA_ARGS__)

namespace cond {

  namespace persistency {

    // helper function to asses the equality of the underlying types, regardless if they are references and their constness
    template <typename T, typename P>
    inline void static_assert_is_same_decayed() {
      static_assert(std::is_same<typename std::decay<T>::type, typename std::decay<P>::type>::value,
                    "Parameter types don't match with the RowBuffer types");
    };

    // functions ( specilized for specific types ) for adding data in AttributeList buffers
    template <typename T>
    inline void f_add_attribute(coral::AttributeList& data,
                                const std::string& attributeName,
                                const T& param,
                                bool init = true) {
      if (init)
        data.extend<T>(attributeName);
      data[attributeName].data<T>() = param;
    }

    template <>
    inline void f_add_attribute(coral::AttributeList& data,
                                const std::string& attributeName,
                                const cond::Binary& param,
                                bool init) {
      if (init)
        data.extend<coral::Blob>(attributeName);
      data[attributeName].bind(param.get());
    }

    template <>
    inline void f_add_attribute(coral::AttributeList& data,
                                const std::string& attributeName,
                                const boost::posix_time::ptime& param,
                                bool init) {
      if (init)
        data.extend<coral::TimeStamp>(attributeName);
      data[attributeName].data<coral::TimeStamp>() = coral::TimeStamp(param);
    }

    template <>
    inline void f_add_attribute(coral::AttributeList& data,
                                const std::string& attributeName,
                                const cond::TimeType& param,
                                bool init) {
      if (init)
        data.extend<std::string>(attributeName);
      data[attributeName].data<std::string>() = cond::time::timeTypeName(param);
    }

    template <>
    inline void f_add_attribute(coral::AttributeList& data,
                                const std::string& attributeName,
                                const cond::SynchronizationType& param,
                                bool init) {
      if (init)
        data.extend<std::string>(attributeName);
      data[attributeName].data<std::string>() = synchronizationTypeNames(param);
    }

    // function for adding into an AttributeList buffer data for a specified column. Performs type checking.
    template <typename Column, typename P>
    inline void f_add_column_data(coral::AttributeList& data, const P& param, bool init = true) {
      static_assert_is_same_decayed<typename Column::type, P>();
      f_add_attribute(data, Column::name, param, init);
    }

    // function for adding into an AttributeList buffer data for a specified condition. Performs type checking.
    template <typename Column, typename P>
    inline void f_add_condition_data(coral::AttributeList& data,
                                     std::string& whereClause,
                                     const P& value,
                                     const std::string condition = "=") {
      static_assert_is_same_decayed<typename Column::type, P>();
      std::stringstream varId;
      unsigned int id = data.size();
      varId << Column::name << "_" << id;
      if (!whereClause.empty())
        whereClause += " AND ";
      whereClause += std::string(Column::fullyQualifiedName()) + " " + condition + " :" + varId.str() + " ";
      //bool init = (id == 0);
      f_add_attribute(data, varId.str(), value);
    }

    // function for appending conditions to a where clause
    template <typename C1, typename C2>
    inline void f_add_condition(std::string& whereClause, const std::string condition = "=") {
      if (!whereClause.empty())
        whereClause += " AND ";
      whereClause += C1::fullyQualifiedName() + " " + condition + " " + C2::fullyQualifiedName() + " ";
    }

    // buffer for data to be inserted into a table
    // maybe better only leave the template set methods ( no class template )
    template <typename... Columns>
    class RowBuffer {
    private:
      template <typename Params, int n, typename T1, typename... Ts>
      void _set(const Params& params, bool init = true) {
        f_add_column_data<T1>(m_data, std::get<n>(params), init);
        _set<Params, n + 1, Ts...>(params, init);
      }

      template <typename Params, int n>
      void _set(const Params&, bool) {}

    public:
      RowBuffer() : m_data() {}

      template <typename P>
      explicit RowBuffer(const P& params) : m_data() {
        _set<P, 0, Columns...>(params);
      }

      template <typename P>
      void set(const P& params) {
        bool init = (m_data.size() == 0);
        // if RowBuffer becames a single type, we need to run either the equivalent of _RowBuffer ( having addAttribute ) when m_data.size()=0, or _set in all other cases
        _set<P, 0, Columns...>(params, init);
      }

      const coral::AttributeList& get() const { return m_data; }

    protected:
      coral::AttributeList m_data;
    };

    template <typename... Columns>
    class ConditionBuffer {
    private:
      template <typename Params, int n, typename T1, typename... Ts>
      void _set(const Params& params) {
        f_add_condition_data<T1>(m_data, m_clause, std::get<n>(params));
        _set<Params, n + 1, Ts...>(params);
      }

      template <typename Params, int n>
      void _set(const Params&) {}

    public:
      ConditionBuffer() : m_data(), m_clause() {}

      template <typename P>
      void set(const P& params) {
        // if RowBuffer becames a single type, we need to run either the equivalent of _RowBuffer ( having addAttribute ) when m_data.size()=0, or _set in all other cases
        _set<P, 0, Columns...>(params);
      }

      void addStaticCondition(const std::string& condition) {
        if (!m_clause.empty()) {
          m_clause += " AND ";
        }
        m_clause += condition;
      }

      const coral::AttributeList& get() const { return m_data; }

      const std::string& getClause() const { return m_clause; }

    protected:
      coral::AttributeList m_data;
      std::string m_clause;
    };

    template <typename T>
    struct AttributeTypeName {
      std::string operator()() { return coral::AttributeSpecification::typeNameForType<T>(); }
    };
    template <>
    struct AttributeTypeName<cond::Binary> {
      std::string operator()() { return coral::AttributeSpecification::typeNameForType<coral::Blob>(); }
    };
    template <>
    struct AttributeTypeName<boost::posix_time::ptime> {
      std::string operator()() { return coral::AttributeSpecification::typeNameForType<coral::TimeStamp>(); }
    };
    template <>
    struct AttributeTypeName<cond::TimeType> {
      std::string operator()() { return coral::AttributeSpecification::typeNameForType<std::string>(); }
    };
    template <>
    struct AttributeTypeName<cond::SynchronizationType> {
      std::string operator()() { return coral::AttributeSpecification::typeNameForType<std::string>(); }
    };

    template <typename T>
    void f_add_column_description(coral::TableDescription& table,
                                  const std::string& columnName,
                                  size_t size = 0,
                                  bool notNull = true) {
      table.insertColumn(columnName, AttributeTypeName<T>()(), size);
      if (notNull)
        table.setNotNullConstraint(columnName);
    }

    template <typename T, typename Arg1>
    constexpr bool is_same_any() {
      return std::is_same<T, Arg1>::value;
    };

    template <typename T, typename Arg1, typename Arg2, typename... Args>
    constexpr bool is_same_any() {
      return is_same_any<T, Arg1>() || is_same_any<T, Arg2, Args...>();
    };

    template <typename... Types>
    class TableDescription {
    private:
      template <int n>
      void addColumn(coral::TableDescription&) {}

      template <int n, typename Arg1, typename... Args>
      void addColumn(coral::TableDescription& tableDescription) {
        std::string columnName(Arg1::name);
        f_add_column_description<typename Arg1::type>(m_description, columnName, Arg1::size);
        addColumn<n + 1, Args...>(m_description);
      }

      template <int, typename Col1, typename... Cols>
      void checkColumns() {
        static_assert(is_same_any<Col1, Types...>(), "Specified Column has not been found in the table.");
        checkColumns<0, Cols...>();
      }

      template <int>
      void checkColumns() {}

    public:
      explicit TableDescription(const char* name) : m_description("ConditionDatabase") {
        m_description.setName(name);
        addColumn<0, Types...>(m_description);
      }

      // for all these methods, we should check that the specified columns belongs to the table columns...
      template <typename... ColumnTypes>
      void setPrimaryKey() {
        checkColumns<0, ColumnTypes...>();
        m_description.setPrimaryKey(makeList<ColumnTypes...>());
      }

      template <typename... ColumnTypes>
      void setUniqueConstraint(const std::string& name) {
        checkColumns<0, ColumnTypes...>();
        m_description.setUniqueConstraint(makeList<ColumnTypes...>(), name);
      }

      template <typename... ColumnTypes>
      void createIndex(const std::string& name) {
        checkColumns<0, ColumnTypes...>();
        m_description.createIndex(name, makeList<ColumnTypes...>());
      }

      template <typename Column, typename ReferencedColumn>
      void setForeignKey(const std::string& name) {
        checkColumns<0, Column>();
        m_description.createForeignKey(
            name, Column::name, std::string(ReferencedColumn::tableName()), ReferencedColumn::name);
      }

      const coral::TableDescription& get() { return m_description; }

    private:
      template <int n>
      void _makeList(std::vector<std::string>&) {}

      template <int n, typename Arg1, typename... Args>
      void _makeList(std::vector<std::string>& columnNames) {
        columnNames.push_back(Arg1::name);
        _makeList<n + 1, Args...>(columnNames);
      }

      template <typename... ColumnTypes>
      std::vector<std::string> makeList() {
        std::vector<std::string> columnList;
        _makeList<0, ColumnTypes...>(columnList);
        return columnList;
      }

    private:
      coral::TableDescription m_description;
    };

    template <typename T>
    struct GetFromRow {
      T operator()(const coral::AttributeList& row, const std::string& fullyQualifiedName) {
        return row[fullyQualifiedName].data<T>();
      }
    };
    template <>
    struct GetFromRow<cond::Binary> {
      cond::Binary operator()(const coral::AttributeList& row, const std::string& fullyQualifiedName) {
        return cond::Binary(row[fullyQualifiedName].data<coral::Blob>());
      }
    };
    template <>
    struct GetFromRow<boost::posix_time::ptime> {
      boost::posix_time::ptime operator()(const coral::AttributeList& row, const std::string& fullyQualifiedName) {
        return row[fullyQualifiedName].data<coral::TimeStamp>().time();
      }
    };
    template <>
    struct GetFromRow<cond::TimeType> {
      cond::TimeType operator()(const coral::AttributeList& row, const std::string& fullyQualifiedName) {
        return cond::time::timeTypeFromName(row[fullyQualifiedName].data<std::string>());
      }
    };
    template <>
    struct GetFromRow<cond::SynchronizationType> {
      cond::SynchronizationType operator()(const coral::AttributeList& row, const std::string& fullyQualifiedName) {
        return cond::synchronizationTypeFromName(row[fullyQualifiedName].data<std::string>());
      }
    };
    template <std::size_t n>
    struct GetFromRow<std::array<char, n>> {
      std::string operator()(const coral::AttributeList& row, const std::string& fullyQualifiedName) {
        std::string val = row[fullyQualifiedName].data<std::string>();
        if (val.size() != n)
          throwException("Retrieved string size does not match with the expected string size.", "getFromRow");
        std::array<char, n> ret;
        ::memcpy(ret.data(), val.c_str(), n);
        return ret;
      }
    };

    template <typename... Types>
    class Query;

    template <typename... Types>
    class QueryIterator {
    public:
      // C++17 compliant iterator definition
      using iterator_category = std::input_iterator_tag;
      using value_type = std::tuple<Types...>;
      using difference_type = void;  // Not used
      using pointer = void;          // Not used
      using reference = void;        // Not used

      QueryIterator() {}

      QueryIterator(const QueryIterator& rhs) : m_query(rhs.m_query), m_currentRow(rhs.m_currentRow) {}

      explicit QueryIterator(Query<Types...>* parent) : m_query(parent) {}

      QueryIterator& operator=(const QueryIterator& rhs) {
        m_query = rhs.m_query;
        m_currentRow = rhs.m_currentRow;
        return *this;
      }

      template <typename T>
      typename T::type get() const {
        return GetFromRow<typename T::type>()(*m_currentRow, std::string(T::fullyQualifiedName()));
      }

      auto operator*() -> decltype(std::make_tuple(this->get<Types>()...)) { return std::make_tuple(get<Types>()...); }

      QueryIterator& operator++() {
        m_currentRow = m_query->next() ? &m_query->currentRow() : nullptr;
        return *this;
      }

      QueryIterator operator++(int) {
        QueryIterator tmp(*this);
        operator++();
        return tmp;
      }

      bool operator==(const QueryIterator& rhs) const {
        if (rhs.m_query == nullptr && m_query == nullptr)
          return true;
        return m_query == rhs.m_query && m_currentRow == rhs.m_currentRow;
      }
      bool operator!=(const QueryIterator& rhs) const { return !operator==(rhs); }

      operator bool() const { return m_currentRow; }

    private:
      Query<Types...>* m_query = nullptr;
      const coral::AttributeList* m_currentRow = nullptr;
    };

    template <typename T>
    struct DefineQueryOutput {
      static void make(coral::IQuery& query, const std::string& fullyQualifiedName) {
        query.addToOutputList(fullyQualifiedName);
        query.defineOutputType(fullyQualifiedName, coral::AttributeSpecification::typeNameForType<T>());
      }
    };
    template <>
    struct DefineQueryOutput<cond::Binary> {
      static void make(coral::IQuery& query, const std::string& fullyQualifiedName) {
        query.addToOutputList(fullyQualifiedName);
        query.defineOutputType(fullyQualifiedName, coral::AttributeSpecification::typeNameForType<coral::Blob>());
      }
    };
    template <>
    struct DefineQueryOutput<boost::posix_time::ptime> {
      static void make(coral::IQuery& query, const std::string& fullyQualifiedName) {
        query.addToOutputList(fullyQualifiedName);
        query.defineOutputType(fullyQualifiedName, coral::AttributeSpecification::typeNameForType<coral::TimeStamp>());
      }
    };
    template <>
    struct DefineQueryOutput<cond::TimeType> {
      static void make(coral::IQuery& query, const std::string& fullyQualifiedName) {
        query.addToOutputList(fullyQualifiedName);
        query.defineOutputType(fullyQualifiedName, coral::AttributeSpecification::typeNameForType<std::string>());
      }
    };
    template <>
    struct DefineQueryOutput<cond::SynchronizationType> {
      static void make(coral::IQuery& query, const std::string& fullyQualifiedName) {
        query.addToOutputList(fullyQualifiedName);
        query.defineOutputType(fullyQualifiedName, coral::AttributeSpecification::typeNameForType<std::string>());
      }
    };
    template <std::size_t n>
    struct DefineQueryOutput<std::array<char, n>> {
      static void make(coral::IQuery& query, const std::string& fullyQualifiedName) {
        query.addToOutputList(fullyQualifiedName);
        query.defineOutputType(fullyQualifiedName, coral::AttributeSpecification::typeNameForType<std::string>());
      }
    };

    template <typename... Types>
    class Query {
    public:
      Query(const coral::ISchema& schema, bool distinct = false)
          : m_coralQuery(schema.newQuery()), m_whereData(), m_whereClause(""), m_tables() {
        _Query<0, Types...>();
        if (distinct)
          m_coralQuery->setDistinct();
      }

      ~Query() {}

      template <typename Col>
      Query& addTable() {
        if (m_tables.find(Col::tableName()) == m_tables.end()) {
          std::string const tb{Col::tableName()};
          m_coralQuery->addToTableList(std::string(tb));
          m_tables.insert(std::move(tb));
        }
        return *this;
      }

      template <int n>
      void _Query() {}

      template <int n, typename Arg1, typename... Args>
      void _Query() {
        addTable<Arg1>();
        DefineQueryOutput<typename Arg1::type>::make(*m_coralQuery, std::string(Arg1::fullyQualifiedName()));
        _Query<n + 1, Args...>();
      }

      template <typename C, typename T>
      Query& addCondition(const T& value, const std::string condition = "=") {
        addTable<C>();
        f_add_condition_data<C>(m_whereData, m_whereClause, value, condition);
        return *this;
      }

      template <typename C1, typename C2>
      Query& addCondition(const std::string condition = "=") {
        addTable<C1>();
        addTable<C2>();
        f_add_condition<C1, C2>(m_whereClause, condition);
        return *this;
      }

      template <typename C>
      Query& addOrderClause(bool ascending = true) {
        std::string orderClause(C::fullyQualifiedName());
        if (!ascending)
          orderClause += " DESC";
        m_coralQuery->addToOrderList(orderClause);
        return *this;
      }

      Query& groupBy(const std::string& expression) {
        m_coralQuery->groupBy(expression);
        return *this;
      }

      Query& setForUpdate() {
        m_coralQuery->setForUpdate();
        return *this;
      }

      Query& limitReturnedRows(size_t nrows) {
        m_coralQuery->limitReturnedRows(nrows);
        return *this;
      }

      bool next() {
        if (!m_cursor)
          throwException("The query has not been executed.", "Query::currentRow");
        bool ret = m_cursor->next();
        if (ret)
          m_retrievedRows++;
        return ret;
      }

      const coral::AttributeList& currentRow() const {
        if (!m_cursor)
          throwException("The query has not been executed.", "Query::currentRow");
        return m_cursor->currentRow();
      }

      const QueryIterator<Types...> begin() {
        m_coralQuery->setCondition(m_whereClause, m_whereData);
        m_cursor = &m_coralQuery->execute();
        m_retrievedRows = 0;
        QueryIterator<Types...> ret(this);
        return ++ret;
      }

      const QueryIterator<Types...> end() { return QueryIterator<Types...>(this); }

      size_t retrievedRows() const { return m_retrievedRows; }

    private:
      std::unique_ptr<coral::IQuery> m_coralQuery;
      coral::ICursor* m_cursor = nullptr;
      size_t m_retrievedRows = 0;
      coral::AttributeList m_whereData;
      std::string m_whereClause;
      std::set<std::string, std::less<>> m_tables;
    };

    class UpdateBuffer {
    private:
      template <typename Params, int n, typename C1, typename... Cs>
      void _set(const Params& params) {
        f_add_column_data<C1>(m_data, std::get<n>(params));
        if (!m_setClause.empty())
          m_setClause += ", ";
        m_setClause += std::string(C1::name) + " = :" + std::string(C1::name);
        _set<Params, n + 1, Cs...>(params);
      }

      template <typename Params, int n>
      void _set(const Params&) {}

    public:
      UpdateBuffer() : m_data(), m_setClause(""), m_whereClause("") {}

      template <typename... Columns, typename Params>
      void setColumnData(const Params& params) {
        _set<Params, 0, Columns...>(params);
      }

      template <typename Column1, typename Column2>
      void setColumnMatch() {
        if (!m_setClause.empty())
          m_setClause += ", ";
        m_setClause += std::string(Column1::name) + " = " + std::string(Column2::name);
      }

      template <typename Column, typename P>
      void addWhereCondition(const P& param, const std::string condition = "=") {
        f_add_condition_data<Column>(m_data, m_whereClause, param, condition);
      }

      template <typename Column1, typename Column2>
      void addWhereCondition(const std::string condition = "=") {
        f_add_condition<Column1, Column2>(m_whereClause, condition);
      }

      const coral::AttributeList& get() const { return m_data; }

      const std::string& setClause() const { return m_setClause; }

      const std::string& whereClause() const { return m_whereClause; }

    private:
      coral::AttributeList m_data;
      std::string m_setClause;
      std::string m_whereClause;
    };

    class DeleteBuffer {
    public:
      DeleteBuffer() : m_data(), m_whereClause("") {}

      template <typename Column, typename P>
      void addWhereCondition(const P& param, const std::string condition = "=") {
        f_add_condition_data<Column>(m_data, m_whereClause, param, condition);
      }

      template <typename Column1, typename Column2>
      void addWhereCondition(const std::string condition = "=") {
        f_add_condition<Column1, Column2>(m_whereClause, condition);
      }

      const coral::AttributeList& get() const { return m_data; }

      const std::string& whereClause() const { return m_whereClause; }

    private:
      coral::AttributeList m_data;
      std::string m_whereClause;
    };

    template <typename... Types>
    class BulkInserter {
    public:
      static constexpr size_t cacheSize = 1000;
      BulkInserter(coral::ISchema& schema, const char* tableName)
          : m_schema(schema), m_tableName(tableName), m_buffer(), m_coralInserter() {
        //fix me: maybe with
        //m_coralInserter.reset(  schema.tableHandle( std::string(tableName ) ).dataEditor().bulkInsert( m_buffer.get(), cacheSize ) );
      }

      template <typename P>
      void insert(const P& params) {
        m_buffer.set(params);
        if (!m_coralInserter.get())
          m_coralInserter.reset(m_schema.tableHandle(m_tableName).dataEditor().bulkInsert(m_buffer.get(), cacheSize));
        m_coralInserter->processNextIteration();
      }

      void flush() {
        if (m_coralInserter.get())
          m_coralInserter->flush();
      }

    private:
      // fixme
      coral::ISchema& m_schema;
      std::string m_tableName;
      //
      RowBuffer<Types...> m_buffer;
      std::unique_ptr<coral::IBulkOperation> m_coralInserter;
    };

    template <typename... Types>
    class BulkDeleter {
    public:
      static constexpr size_t cacheSize = 1000;
      explicit BulkDeleter(coral::ISchema& schema, const char* tableName)
          : m_schema(schema), m_tableName(tableName), m_buffer(), m_coralDeleter() {
        //fix me: maybe with
        //m_coralInserter.reset(  schema.tableHandle( std::string(tableName ) ).dataEditor().bulkInsert( m_buffer.get(), cacheSize ) );
      }

      void addStaticCondition(const std::string& condition) { m_buffer.addStaticCondition(condition); }

      template <typename P>
      void erase(const P& params) {
        m_buffer.set(params);
        if (!m_coralDeleter.get())
          m_coralDeleter.reset(m_schema.tableHandle(m_tableName)
                                   .dataEditor()
                                   .bulkDeleteRows(m_buffer.getClause(), m_buffer.get(), cacheSize));
        m_coralDeleter->processNextIteration();
      }

      void flush() {
        if (m_coralDeleter.get())
          m_coralDeleter->flush();
      }

    private:
      // fixme
      coral::ISchema& m_schema;
      std::string m_tableName;
      //
      ConditionBuffer<Types...> m_buffer;
      std::unique_ptr<coral::IBulkOperation> m_coralDeleter;
    };

    namespace {

      inline bool existsTable(coral::ISchema& schema, const char* tableName) {
        return schema.existsTable(std::string(tableName));
      }

      inline void createTable(coral::ISchema& schema, const coral::TableDescription& descr) {
        schema.createTable(descr);
      }

      inline bool insertInTable(coral::ISchema& schema,
                                const char* tableName,
                                const coral::AttributeList& row,
                                bool failOnDuplicate = true) {
        bool ret = false;
        try {
          schema.tableHandle(std::string(tableName)).dataEditor().insertRow(row);
          ret = true;
        } catch (const coral::DuplicateEntryInUniqueKeyException&) {
          if (failOnDuplicate)
            throw;
        }
        return ret;
      }

      inline void updateTable(coral::ISchema& schema, const char* tableName, const UpdateBuffer& data) {
        schema.tableHandle(std::string(tableName))
            .dataEditor()
            .updateRows(data.setClause(), data.whereClause(), data.get());
      }

      inline void deleteFromTable(coral::ISchema& schema, const char* tableName, const DeleteBuffer& data) {
        schema.tableHandle(std::string(tableName)).dataEditor().deleteRows(data.whereClause(), data.get());
      }
    }  // namespace

  }  // namespace persistency

}  // namespace cond

#endif
