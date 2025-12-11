# Search System Analysis and Recommendations

## Current Implementation Analysis

### Search Components in Codebase
The current search implementation is primarily located in `src/searchdialog.cpp` and consists of:

1. **MySQL MATCH() AGAINST()** for NFe tables (line 123)
   ```cpp
   searchFilter = "MATCH(" + filtro1.join(", ") + ") AGAINST('" + filtro2.join(" ") + "' IN BOOLEAN MODE) ORDER BY numeroNFe";
   ```

2. **Basic LIKE '%term%'** for other searches (line 132)
   ```cpp
   parteFiltro << fullTextIndexes.at(i).index + " LIKE '%" + qApp->sanitizeSQL(lineEdits.at(i)->text()) + "%'";
   ```

3. **SearchDialog class structure** supporting multiple entity types:
   - Cliente (Customer)
   - Produto (Product) 
   - Fornecedor (Supplier)
   - NFe (Electronic Invoice)
   - And others

### Current Limitations Identified

1. **MySQL Fulltext Prefix Limitation**: Cannot find substrings without leading wildcards
   - Example: Typing "2345" won't find "12345"
   - TODO comment in code acknowledges this: "MySQL Full-Text doesn't support prefix, use Sphinx/Manticore instead"

2. **No Fuzzy Search**: Cannot handle typos or similar spellings
   - Example: "stacato" won't find "staccato"

3. **Limited Substring Matching**: Current LIKE queries are slow on large datasets
   - Performance degrades with database size

4. **Language-Specific Issues**: Portuguese text with accents and special characters
   - CPF formatting issues mentioned in code comments

## Problem Statement

The main limitation is **lack of wildcard prefix support in MySQL fulltext**, preventing users from finding:
- Partial product codes (2345 → 12345)
- Names with missing prefixes (john → johnson) 
- Misspelled terms with reasonable tolerance

The database is too large for client-side search solutions.

## Recommended Solutions

### 1. Sphinx/Manticore Search (Primary Recommendation)

**Why this is the optimal choice:**
- Already identified in codebase TODO comments
- Designed specifically for MySQL integration
- Handles exact use case: substring, prefix, and fuzzy search
- Built for large dataset performance
- Minimal disruption to existing architecture

**Capabilities:**
```sql
-- Substring search with wildcards
SELECT * FROM products WHERE MATCH('*2345*');  -- finds 12345

-- Prefix search  
SELECT * FROM products WHERE MATCH('john*');   -- finds johnson

-- Fuzzy search with edit distance
SELECT * FROM products WHERE MATCH('stacato~2'); -- finds staccato

-- Boolean queries
SELECT * FROM products WHERE MATCH('"staccato produto" | "produto staccato"');
```

**Integration approach:**
```cpp
// Minimal changes to SearchDialog::on_lineEditBusca_textChanged()
QString searchFilter;

if (useSphinxSearch && text.length() >= 3) {
    // Sphinx query with wildcards and fuzzy matching
    QString sphinxQuery = "*" + qApp->sanitizeSQL(text) + "*";
    searchFilter = "MATCH('" + sphinxQuery + "')";
} else {
    // Fallback to current LIKE method for short terms
    searchFilter = fullTextIndexes.at(i).index + " LIKE '%" + qApp->sanitizeSQL(text) + "%'";
}
```

**Implementation phases:**
1. Install Manticore Search service
2. Configure indexing from existing MySQL tables
3. Modify SearchDialog to use Sphinx for complex searches
4. Keep MySQL for simple exact matches and fallback

### 2. PostgreSQL Migration (Long-term Alternative)

**Advantages:**
- Native trigram support for fuzzy search
- Excellent full-text search capabilities
- Better substring matching performance

**Example capabilities:**
```sql
CREATE EXTENSION pg_trgm;
CREATE INDEX idx_produto_descricao_trgm ON produto USING gin(descricao gin_trgm_ops);

-- Fuzzy matching
SELECT * FROM produto WHERE descricao % 'stacato';  -- finds staccato

-- Fast substring search  
SELECT * FROM produto WHERE descricao ILIKE '%2345%'; -- fast with trigram index
```

**Considerations:**
- Requires database migration
- More disruptive to existing system
- Long-term strategic decision

### 3. MySQL Workaround with Reverse Indexing (If Staying with MySQL)

**Approach:**
Create auxiliary search tables with pre-computed substrings:

```sql
-- Pre-compute all substrings for fast lookup
CREATE TABLE produto_search_terms (
    idProduto INT,
    search_term VARCHAR(255),
    term_type ENUM('codigo', 'descricao', 'fornecedor'),
    INDEX(search_term),
    FOREIGN KEY(idProduto) REFERENCES produto(idProduto)
);

-- Populate with all substrings
INSERT INTO produto_search_terms 
SELECT idProduto, SUBSTRING(codComercial, i, LENGTH(codComercial)) as search_term, 'codigo'
FROM produto 
CROSS JOIN (SELECT 1 as i UNION SELECT 2 UNION SELECT 3 ...) numbers
WHERE i <= LENGTH(codComercial);
```

**Disadvantages:**
- Increased storage requirements
- Complex maintenance
- Still doesn't solve fuzzy search

### 4. Elasticsearch (Enterprise-Grade Alternative)

**Advantages:**
- Industry-standard search engine
- Excellent fuzzy matching and analytics
- Highly scalable

**Integration example:**
```cpp
class ElasticsearchClient {
private:
    QNetworkAccessManager *manager;
    QString elasticsearchUrl;
    
public:
    QJsonDocument fuzzySearch(const QString &index, const QString &query, int fuzziness = 2);
    QJsonDocument substringSearch(const QString &index, const QString &query);
};
```

**Disadvantages:**
- Additional infrastructure complexity
- More resource intensive
- Overkill for current requirements

## Hybrid Implementation Strategy

**Recommended approach combining multiple techniques:**

```cpp
class ImprovedSearchDialog : public SearchDialog {
private:
    enum SearchMode {
        MySQL_Exact,      // For exact matches
        MySQL_Fulltext,   // For existing fulltext indexes  
        Sphinx_Advanced   // For complex substring/fuzzy search
    };
    
public slots:
    void performIntelligentSearch(const QString &query);
    
private:
    SearchMode determineSearchMode(const QString &query);
    QString buildSphinxQuery(const QString &query);
    QString buildMySQLQuery(const QString &query);
};
```

**Search logic:**
1. **Short terms (< 3 chars)**: Use MySQL exact match
2. **Existing fulltext fields**: Use current MySQL MATCH() AGAINST()
3. **Complex substring/fuzzy needs**: Use Sphinx/Manticore
4. **Cache popular searches** for performance

## Implementation Priority

### Phase 1: Sphinx/Manticore Integration (Immediate)
- Install and configure Manticore Search
- Create indexes for critical tables (produto, cliente, fornecedor)
- Modify SearchDialog for hybrid search approach
- Test with subset of search scenarios

### Phase 2: Enhanced Search Features (Short-term)
- Add fuzzy search tolerance configuration
- Implement search result ranking
- Add search analytics and optimization
- Handle Portuguese language specifics (accents, formatting)

### Phase 3: Advanced Features (Long-term)
- Search suggestions and autocomplete
- Search result highlighting
- Advanced filtering combinations
- Performance monitoring and tuning

## Code Integration Points

### Files to Modify:
- `src/searchdialog.cpp` - Core search logic
- `src/searchdialog.h` - Add Sphinx client integration
- Search-specific methods in entity classes

### New Components Needed:
- Sphinx/Manticore client wrapper class
- Search configuration management
- Index management utilities
- Search performance monitoring

## Expected Benefits

1. **Immediate**: Find "12345" when typing "2345"
2. **Short-term**: Handle common typos and variations
3. **Long-term**: Scalable search architecture for growing dataset
4. **User Experience**: Faster, more intuitive search results

## Risk Mitigation

- **Fallback Strategy**: Keep existing MySQL search as backup
- **Gradual Rollout**: Implement on subset of tables first
- **Performance Monitoring**: Track search response times
- **User Training**: Document new search capabilities

## MySQL-to-Manticore Synchronization Challenge

### The Reliability Problem
Manual synchronization in application code is inherently unreliable:
- Easy to forget sync calls in some code paths
- Bulk operations may skip sync steps
- Exception handling can leave indexes out of sync
- Developer discipline required across entire team

### Solution: Change Data Capture (CDC) Tools

Rather than manually parsing binlogs and managing queues, there are mature, production-ready tools designed specifically for database change synchronization:

## CDC Tools for MySQL-to-Manticore Sync

### 1. **Debezium** (Primary Recommendation)

**Production-grade CDC platform used by Netflix, Uber, LinkedIn**

**Architecture:**
```
Your Qt ERP → MySQL → Debezium → Kafka → Manticore Connect → Manticore Search
```

**Key advantages:**
- **Zero application changes** - Your Qt code remains untouched
- **Exactly-once delivery** - No duplicate or missed syncs
- **Schema evolution** - Handles table structure changes automatically
- **Failure recovery** - Resumes from exact binlog position after restarts
- **Built-in monitoring** - Metrics and health checks included

**Configuration example:**
```json
{
  "name": "mysql-connector",
  "config": {
    "connector.class": "io.debezium.connector.mysql.MySqlConnector",
    "database.hostname": "localhost",
    "database.port": "3306",
    "database.user": "debezium",
    "database.password": "password",
    "database.server.id": "184054",
    "database.server.name": "erp_staccato",
    "database.include.list": "erp_staccato",
    "table.include.list": "erp_staccato.produto,erp_staccato.cliente,erp_staccato.fornecedor",
    "database.history.kafka.bootstrap.servers": "localhost:9092",
    "database.history.kafka.topic": "erp_schema_changes"
  }
}
```

### 2. **Maxwell's Daemon** (Lightweight Alternative)

**Simple MySQL binlog to JSON stream:**

```bash
# Install and configure Maxwell
cat > config.properties << EOF
host=localhost
user=maxwell
password=password
schema_database=maxwell
client_id=maxwell_erp
database_include_list=erp_staccato
table_include_list=produto,cliente,fornecedor
output_ddl=true
EOF

# Run - outputs JSON events
bin/maxwell --config=config.properties --producer=http --http_url=http://localhost:8080/sync
```

**Change event example:**
```json
{
  "database": "erp_staccato",
  "table": "produto", 
  "type": "update",
  "ts": 1642678901,
  "data": {
    "idProduto": 12345,
    "descricao": "Mesa Staccato Premium",
    "codComercial": "MST001"
  },
  "old": {
    "descricao": "Mesa Staccato"
  }
}
```

**Simple Qt HTTP receiver:**
```cpp
class MaxwellReceiver : public QHttpServer {
public:
    MaxwellReceiver() {
        route("/sync", QHttpServerRequest::Method::Post, [this](const QHttpServerRequest &request) {
            QJsonDocument doc = QJsonDocument::fromJson(request.body());
            QJsonObject event = doc.object();
            
            QString table = event["table"].toString();
            QString type = event["type"].toString();
            QJsonObject data = event["data"].toObject();
            
            if (table == "produto") {
                syncToManticore(data, type);
            }
            
            return QHttpServerResponse("OK");
        });
    }
};
```

### 3. **Canal** (Alibaba's MySQL CDC)

**High-performance MySQL replication protocol:**
```yaml
# canal.properties
canal.destinations = manticore
canal.instance.mysql.slaveId = 1234
canal.instance.master.address = localhost:3306
canal.instance.dbUsername = canal
canal.instance.dbPassword = password
canal.instance.filter.regex = erp_staccato\\.produto,erp_staccato\\.cliente
```

### 4. **Logstash** (Part of ELK Stack)

**The tool you were thinking of! Logstash can sync MySQL to anywhere:**

```ruby
# logstash-mysql-to-manticore.conf
input {
  jdbc {
    jdbc_driver_library => "/path/to/mysql-connector-java.jar"
    jdbc_driver_class => "com.mysql.cj.jdbc.Driver"
    jdbc_connection_string => "jdbc:mysql://localhost:3306/erp_staccato"
    jdbc_user => "logstash"
    jdbc_password => "password"
    statement => "SELECT idProduto, descricao, codComercial, fornecedor, colecao FROM produto WHERE updated_at > :sql_last_value"
    use_column_value => true
    tracking_column => "updated_at"
    tracking_column_type => "timestamp"
    schedule => "*/30 * * * * *"  # Every 30 seconds
  }
}

output {
  manticore {
    hosts => ["localhost:9308"]
    index => "produtos_rt"
    action => "replace"
    document_id => "%{idproduto}"
  }
}
```

### 5. **Vector** (Modern Logstash Alternative)

**Rust-based, high-performance data pipeline:**
```toml
# vector.toml
[sources.mysql_produtos]
type = "mysql_metrics"
endpoints = ["mysql://user:pass@localhost:3306/erp_staccato"]

[transforms.format_for_manticore]
type = "remap"
inputs = ["mysql_produtos"]
source = '''
.id = .idProduto
del(.idProduto)
'''

[sinks.manticore]
type = "http"
inputs = ["format_for_manticore"]
uri = "http://localhost:9308/sql"
method = "post"
```

## Recommended Implementation Strategy

### **Phase 1: Proof of Concept with Maxwell**
1. **Quick setup** - Single binary, minimal configuration
2. **HTTP endpoint** - Easy to integrate with Qt
3. **Immediate results** - See changes flowing in real-time
4. **Low commitment** - Easy to switch if needed

### **Phase 2: Production with Debezium**
1. **Enterprise reliability** - Battle-tested by major companies  
2. **Full ecosystem** - Kafka Connect sinks available for Manticore
3. **Operational maturity** - Monitoring, scaling, failure recovery
4. **Future-proof** - Handles growth and additional data sources

### **Docker Compose Setup** (Complete Stack)
```yaml
version: '3.8'
services:
  mysql:
    image: mysql:8.0
    environment:
      MYSQL_ROOT_PASSWORD: password
    command: --log-bin=mysql-bin --server-id=1 --binlog-format=ROW

  manticore:
    image: manticoresearch/manticore
    ports:
      - "9308:9308"

  kafka:
    image: confluentinc/cp-kafka
    environment:
      KAFKA_ZOOKEEPER_CONNECT: zookeeper:2181
      KAFKA_ADVERTISED_LISTENERS: PLAINTEXT://localhost:9092

  debezium:
    image: debezium/connect
    environment:
      BOOTSTRAP_SERVERS: kafka:9092
      GROUP_ID: 1
      CONFIG_STORAGE_TOPIC: debezium_configs
      OFFSET_STORAGE_TOPIC: debezium_offsets
    depends_on: [kafka]

  # Your Qt ERP continues running normally - zero changes needed!
```

**Single command deployment:**
```bash
docker-compose up -d
```

## CDC Benefits Over Manual Sync

✅ **100% Reliability** - Captures every database change automatically  
✅ **Zero Code Changes** - Your Qt ERP works exactly as before  
✅ **Automatic Recovery** - Resumes from exact position after failures  
✅ **Schema Evolution** - Handles table structure changes gracefully  
✅ **Built-in Monitoring** - Health checks and metrics included  
✅ **High Performance** - Optimized for high-volume changes  
✅ **Exactly-Once Semantics** - No duplicates, no missed changes  
✅ **Production Proven** - Used by major tech companies  

## Updated Conclusion

**Change Data Capture tools like Debezium or Maxwell** solve the synchronization reliability problem completely. They provide enterprise-grade reliability without any changes to your Qt application code.

**Recommended approach:**
1. **Immediate**: Start with Maxwell for quick proof of concept
2. **Production**: Upgrade to Debezium + Kafka for enterprise reliability
3. **Search**: Use Manticore for advanced substring and fuzzy search capabilities

This combination gives you both reliable synchronization AND powerful search features, solving your original "find 12345 when typing 2345" problem with production-grade infrastructure.