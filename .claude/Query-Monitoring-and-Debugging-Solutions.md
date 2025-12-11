# MySQL Query Monitoring & Debugging Solutions

## Current Problem with MySQL general_log

### Limitations Identified
- **High noise**: Captures every query including internal MySQL operations
- **No user context**: Only shows connection ID, not actual application user  
- **No filtering capability**: Can't isolate specific users or operations
- **No real-time monitoring**: Static log files, difficult to monitor live
- **Performance impact**: Can significantly slow down MySQL
- **Difficult analysis**: Raw text format, hard to parse and filter
- **Volume issues**: Even with few dozen users, generates overwhelming amount of data

### Specific Pain Points for ERP Debugging
- Cannot trace which application user executed problematic queries
- Unable to filter queries by specific modules or operations
- No real-time visibility into user actions for testing/debugging
- Difficult to correlate queries with specific business operations
- Hard to identify performance bottlenecks by user or module

## Superior Monitoring Solutions

### 1. Application-Level Structured Logging (Immediate Solution)

**Implement rich context logging directly in Qt application:**

```cpp
class QueryLogger : public QObject {
    Q_OBJECT
    
private:
    QFile logFile;
    QTextStream logStream;
    QString currentUser;
    QString currentModule;
    bool realTimeEnabled;
    
public:
    QueryLogger(QObject *parent = nullptr) : QObject(parent), realTimeEnabled(false) {
        logFile.setFileName("queries.jsonl");
        if (logFile.open(QIODevice::WriteOnly | QIODevice::Append)) {
            logStream.setDevice(&logFile);
        }
    }
    
    void setContext(const QString &user, const QString &module) {
        currentUser = user;
        currentModule = module;
    }
    
    void setLogFile(const QString &filename) {
        if (logFile.isOpen()) {
            logFile.close();
        }
        logFile.setFileName(filename);
        logFile.open(QIODevice::WriteOnly | QIODevice::Append);
        logStream.setDevice(&logFile);
    }
    
    void enableRealTime(bool enabled) {
        realTimeEnabled = enabled;
    }
    
    void logQuery(const QString &query, qint64 executionTime = -1) {
        QJsonObject logEntry;
        logEntry["timestamp"] = QDateTime::currentDateTime().toString(Qt::ISODate);
        logEntry["user"] = currentUser;
        logEntry["module"] = currentModule;
        logEntry["query"] = query;
        logEntry["execution_time_ms"] = executionTime;
        logEntry["thread_id"] = QThread::currentThreadId();
        logEntry["connection_id"] = QSqlDatabase::database().connectionName();
        
        // Add query categorization
        if (query.contains("SELECT", Qt::CaseInsensitive)) {
            logEntry["query_type"] = "SELECT";
        } else if (query.contains("INSERT", Qt::CaseInsensitive)) {
            logEntry["query_type"] = "INSERT";
        } else if (query.contains("UPDATE", Qt::CaseInsensitive)) {
            logEntry["query_type"] = "UPDATE";
        } else if (query.contains("DELETE", Qt::CaseInsensitive)) {
            logEntry["query_type"] = "DELETE";
        }
        
        // Add table detection
        QStringList tables = {"produto", "cliente", "fornecedor", "venda", "estoque"};
        for (const QString &table : tables) {
            if (query.contains(table, Qt::CaseInsensitive)) {
                logEntry["tables"] = QJsonArray::fromStringList({table});
                break;
            }
        }
        
        // Write to structured log
        logStream << QJsonDocument(logEntry).toJson(QJsonDocument::Compact) << "\n";
        logStream.flush();
        
        // Send to real-time stream if enabled
        if (realTimeEnabled) {
            emit queryLogged(logEntry);
        }
    }
    
signals:
    void queryLogged(const QJsonObject &entry);
};
```

**Integration with existing SqlQuery wrapper:**

```cpp
// Enhanced SqlQuery class with logging
class SqlQuery : public QSqlQuery {
private:
    static QueryLogger *logger;
    
public:
    SqlQuery(QSqlDatabase db = QSqlDatabase()) : QSqlQuery(db) {}
    
    bool exec(const QString &query) override {
        QElapsedTimer timer;
        timer.start();
        
        bool result = QSqlQuery::exec(query);
        
        // Log with execution time and context
        if (logger) {
            logger->logQuery(query, timer.elapsed());
        }
        
        return result;
    }
    
    bool exec() override {
        QElapsedTimer timer;
        timer.start();
        
        bool result = QSqlQuery::exec();
        
        // Log prepared statement with execution time
        if (logger) {
            logger->logQuery(lastQuery(), timer.elapsed());
        }
        
        return result;
    }
    
    static void setLogger(QueryLogger *queryLogger) {
        logger = queryLogger;
    }
};

// Static member definition
QueryLogger* SqlQuery::logger = nullptr;
```

**Application class integration:**

```cpp
class Application : public QApplication {
private:
    QueryLogger *queryLogger;
    
public:
    Application(int argc, char *argv[]) : QApplication(argc, argv) {
        // Initialize query logging
        queryLogger = new QueryLogger(this);
        queryLogger->setLogFile("erp_queries.jsonl");
        
        // Set up SqlQuery logging
        SqlQuery::setLogger(queryLogger);
        
        // Set user context when user logs in
        connect(this, &Application::userLoggedIn, [this](const QString &user) {
            queryLogger->setContext(user, "ERP");
        });
    }
    
    // Context management methods
    void setQueryContext(const QString &module) {
        if (queryLogger) {
            queryLogger->setContext(User::nome, module);
        }
    }
    
    // Make logger globally accessible
    static QueryLogger* getQueryLogger() { 
        return qobject_cast<Application*>(instance())->queryLogger; 
    }
};
```

**Usage in existing modules:**

```cpp
// In SearchDialog constructor
SearchDialog::SearchDialog(...) {
    qApp->setQueryContext("SearchDialog");
    // ... existing code
}

// In CadastroCliente
void CadastroCliente::on_pushButtonSalvar_clicked() {
    qApp->setQueryContext("CadastroCliente::Save");
    
    // ... existing save logic ...
    // All SQL queries will now be logged with proper context
}
```

### 2. ProxySQL (Database-Level Solution)

**Professional database proxy with comprehensive monitoring:**

**Installation:**
```bash
# Install ProxySQL
wget https://github.com/sysown/proxysql/releases/download/v2.4.4/proxysql_2.4.4-1_amd64.deb
sudo dpkg -i proxysql_2.4.4-1_amd64.deb

# Start ProxySQL
sudo systemctl start proxysql
sudo systemctl enable proxysql
```

**Configuration:**
```sql
-- Connect to ProxySQL Admin interface
mysql -u admin -padmin -h 127.0.0.1 -P6032

-- Configure MySQL backend servers
INSERT INTO mysql_servers(hostgroup_id, hostname, port, weight) VALUES
(0, '127.0.0.1', 3306, 1000);

-- Configure users
INSERT INTO mysql_users(username, password, default_hostgroup) VALUES 
('erp_user', 'password', 0);

-- Configure query logging rules
INSERT INTO mysql_query_rules(rule_id, match_pattern, destination_hostgroup, apply, log) VALUES
(1, 'SELECT.*produto.*', 0, 1, 1),          -- Log product queries
(2, 'SELECT.*cliente.*', 0, 1, 1),          -- Log client queries  
(3, 'INSERT.*', 0, 1, 1),                   -- Log all inserts
(4, 'UPDATE.*', 0, 1, 1),                   -- Log all updates
(5, 'DELETE.*', 0, 1, 1);                   -- Log all deletes

-- Enable comprehensive logging
SET mysql-eventslog_filename='erp_queries.log';
SET mysql-eventslog_default_log=1;
SET mysql-eventslog_format=2;  -- Detailed JSON format

-- Load configuration
LOAD MYSQL SERVERS TO RUNTIME;
LOAD MYSQL USERS TO RUNTIME; 
LOAD MYSQL QUERY RULES TO RUNTIME;
SAVE MYSQL SERVERS TO DISK;
SAVE MYSQL USERS TO DISK;
SAVE MYSQL QUERY RULES TO DISK;
```

**Real-time monitoring:**
```bash
# Watch queries in real-time with filtering
tail -f /var/lib/proxysql/erp_queries.log | jq 'select(.username == "erp_user")'

# Filter by query type
tail -f /var/lib/proxysql/erp_queries.log | jq 'select(.query | contains("produto"))'
```

### 3. MySQL Performance Schema Enhanced Monitoring

**Comprehensive Performance Schema setup:**

```sql
-- Enable Performance Schema (add to my.cnf)
[mysqld]
performance_schema = ON
performance-schema-consumer-events-statements-current = ON
performance-schema-consumer-events-statements-history = ON
performance-schema-consumer-events-statements-history-long = ON
performance-schema-consumer-events-waits-current = ON
```

**Advanced monitoring queries:**

```sql
-- Real-time query monitoring with user context
CREATE VIEW current_user_queries AS
SELECT 
    pps.PROCESSLIST_ID as connection_id,
    pps.PROCESSLIST_USER as user,
    pps.PROCESSLIST_DB as database_name,
    pps.PROCESSLIST_INFO as current_query,
    pps.PROCESSLIST_TIME as duration_seconds,
    pps.PROCESSLIST_STATE as state,
    pps.PROCESSLIST_HOST as client_host
FROM performance_schema.processlist pps
WHERE pps.PROCESSLIST_COMMAND = 'Query'
AND pps.PROCESSLIST_USER != 'system user';

-- Historical query analysis
CREATE VIEW query_history_analysis AS
SELECT 
    esh.SQL_TEXT as query,
    COUNT(*) as execution_count,
    AVG(esh.TIMER_WAIT/1000000000) as avg_duration_seconds,
    MAX(esh.TIMER_WAIT/1000000000) as max_duration_seconds,
    SUM(esh.ROWS_EXAMINED) as total_rows_examined,
    esh.EVENT_NAME as event_type
FROM performance_schema.events_statements_history_long esh
GROUP BY esh.SQL_TEXT, esh.EVENT_NAME
ORDER BY execution_count DESC;

-- Slow query identification
SELECT 
    SUBSTRING(esh.SQL_TEXT, 1, 100) as query_preview,
    esh.TIMER_WAIT/1000000000 as duration_seconds,
    esh.ROWS_EXAMINED,
    esh.ROWS_SENT,
    esh.TIMER_START
FROM performance_schema.events_statements_history_long esh
WHERE esh.TIMER_WAIT/1000000000 > 1.0  -- Queries longer than 1 second
ORDER BY esh.TIMER_WAIT DESC
LIMIT 20;
```

**Qt Performance Schema monitor:**

```cpp
class PerformanceSchemaMonitor : public QObject {
    Q_OBJECT
    
private:
    QTimer *refreshTimer;
    QSqlDatabase monitorDb;
    QString targetUser;
    
public:
    PerformanceSchemaMonitor(const QString &user = "", QObject *parent = nullptr) 
        : QObject(parent), targetUser(user) {
        monitorDb = QSqlDatabase::addDatabase("QMYSQL", "monitor");
        monitorDb.setHostName("localhost");
        monitorDb.setDatabaseName("performance_schema");
        monitorDb.setUserName("monitor_user");
        monitorDb.setPassword("monitor_password");
        
        refreshTimer = new QTimer(this);
        connect(refreshTimer, &QTimer::timeout, this, &PerformanceSchemaMonitor::refreshQueries);
        refreshTimer->start(2000); // Refresh every 2 seconds
    }
    
public slots:
    void refreshQueries() {
        if (!monitorDb.isOpen()) {
            monitorDb.open();
        }
        
        SqlQuery query(monitorDb);
        QString sql = R"(
            SELECT 
                pps.PROCESSLIST_USER as user,
                SUBSTRING(esh.SQL_TEXT, 1, 200) as query,
                esh.TIMER_WAIT/1000000000 as duration_seconds,
                esh.ROWS_EXAMINED,
                NOW() as captured_at
            FROM performance_schema.events_statements_history esh
            JOIN performance_schema.processlist pps ON esh.THREAD_ID = pps.THREAD_ID
            WHERE esh.TIMER_START > UNIX_TIMESTAMP(NOW() - INTERVAL 10 SECOND) * 1000000000000
        )";
        
        if (!targetUser.isEmpty()) {
            sql += QString(" AND pps.PROCESSLIST_USER = '%1'").arg(targetUser);
        }
        
        query.exec(sql);
        
        while (query.next()) {
            emit queryDetected(
                query.value("user").toString(),
                query.value("query").toString(),
                query.value("duration_seconds").toDouble(),
                query.value("rows_examined").toInt()
            );
        }
    }
    
    void setTargetUser(const QString &user) {
        targetUser = user;
    }
    
signals:
    void queryDetected(const QString &user, const QString &query, 
                      double duration, int rowsExamined);
};
```

### 4. Real-Time Query Streaming & Dashboard

**WebSocket-based real-time monitoring:**

```cpp
class QueryStreamServer : public QWebSocketServer {
    Q_OBJECT
    
private:
    QList<QWebSocket*> clients;
    QueryLogger *logger;
    QJsonObject clientFilters; // Per-client filtering
    
public:
    QueryStreamServer(QueryLogger *queryLogger, QObject *parent = nullptr) 
        : QWebSocketServer("QueryStream", QWebSocketServer::NonSecureMode, parent),
          logger(queryLogger) {
        connect(logger, &QueryLogger::queryLogged, this, &QueryStreamServer::broadcastQuery);
        connect(this, &QWebSocketServer::newConnection, this, &QueryStreamServer::onNewConnection);
        
        listen(QHostAddress::Any, 8080);
    }
    
private slots:
    void broadcastQuery(const QJsonObject &query) {
        QJsonDocument doc(query);
        QString message = doc.toJson();
        
        for (auto *client : clients) {
            // Apply per-client filtering
            if (shouldSendToClient(client, query)) {
                client->sendTextMessage(message);
            }
        }
    }
    
    void onNewConnection() {
        QWebSocket *socket = nextPendingConnection();
        clients.append(socket);
        
        connect(socket, &QWebSocket::textMessageReceived, [this, socket](const QString &message) {
            handleClientMessage(socket, message);
        });
        
        connect(socket, &QWebSocket::disconnected, [this, socket]() {
            clients.removeAll(socket);
            clientFilters.remove(QString::number(reinterpret_cast<qint64>(socket)));
            socket->deleteLater();
        });
        
        // Send welcome message
        socket->sendTextMessage(R"({"type":"welcome","message":"Connected to query stream"})");
    }
    
private:
    bool shouldSendToClient(QWebSocket *client, const QJsonObject &query) {
        QString clientKey = QString::number(reinterpret_cast<qint64>(client));
        if (!clientFilters.contains(clientKey)) {
            return true; // No filter set, send everything
        }
        
        QJsonObject filter = clientFilters[clientKey].toObject();
        
        // Apply user filter
        if (filter.contains("user") && !filter["user"].toString().isEmpty()) {
            if (!query["user"].toString().contains(filter["user"].toString(), Qt::CaseInsensitive)) {
                return false;
            }
        }
        
        // Apply query filter
        if (filter.contains("query") && !filter["query"].toString().isEmpty()) {
            if (!query["query"].toString().contains(filter["query"].toString(), Qt::CaseInsensitive)) {
                return false;
            }
        }
        
        // Apply minimum duration filter
        if (filter.contains("min_duration") && filter["min_duration"].toDouble() > 0) {
            if (query["execution_time_ms"].toDouble() < filter["min_duration"].toDouble()) {
                return false;
            }
        }
        
        return true;
    }
    
    void handleClientMessage(QWebSocket *client, const QString &message) {
        QJsonDocument doc = QJsonDocument::fromJson(message.toUtf8());
        QJsonObject obj = doc.object();
        
        if (obj["type"].toString() == "filter") {
            QString clientKey = QString::number(reinterpret_cast<qint64>(client));
            clientFilters[clientKey] = obj["filter"];
            
            client->sendTextMessage(R"({"type":"filter_applied","status":"success"})");
        }
    }
};
```

**HTML Dashboard:**

```html
<!DOCTYPE html>
<html>
<head>
    <title>ERP Real-Time Query Monitor</title>
    <style>
        body { font-family: Arial, sans-serif; margin: 20px; }
        .filters { background: #f5f5f5; padding: 15px; margin-bottom: 20px; border-radius: 5px; }
        .filter-group { margin-right: 20px; display: inline-block; }
        .query-entry { border: 1px solid #ddd; margin: 5px 0; padding: 10px; border-radius: 3px; }
        .query-meta { color: #666; font-size: 12px; margin-bottom: 5px; }
        .query-sql { font-family: monospace; background: #f8f8f8; padding: 5px; white-space: pre-wrap; }
        .slow-query { border-left: 4px solid #ff4444; }
        .fast-query { border-left: 4px solid #44ff44; }
        .stats { background: #e8f4f8; padding: 10px; margin-bottom: 20px; border-radius: 5px; }
    </style>
</head>
<body>
    <h1>ERP Real-Time Query Monitor</h1>
    
    <div class="stats">
        <span>Connected: <span id="connectionStatus">Disconnected</span></span>
        <span style="margin-left: 20px;">Total Queries: <span id="queryCount">0</span></span>
        <span style="margin-left: 20px;">Avg Response Time: <span id="avgTime">0ms</span></span>
    </div>
    
    <div class="filters">
        <div class="filter-group">
            <label>Filter by User:</label>
            <input type="text" id="userFilter" placeholder="Enter username">
        </div>
        <div class="filter-group">
            <label>Filter by Query:</label>
            <input type="text" id="queryFilter" placeholder="Enter query text">
        </div>
        <div class="filter-group">
            <label>Min Duration (ms):</label>
            <input type="number" id="durationFilter" placeholder="0" min="0">
        </div>
        <button onclick="applyFilters()">Apply Filters</button>
        <button onclick="clearFilters()">Clear Filters</button>
    </div>
    
    <div id="queries"></div>

    <script>
        let ws = null;
        let queryCount = 0;
        let totalTime = 0;
        
        function connect() {
            ws = new WebSocket('ws://localhost:8080');
            
            ws.onopen = function() {
                document.getElementById('connectionStatus').textContent = 'Connected';
                document.getElementById('connectionStatus').style.color = 'green';
            };
            
            ws.onclose = function() {
                document.getElementById('connectionStatus').textContent = 'Disconnected';
                document.getElementById('connectionStatus').style.color = 'red';
                
                // Reconnect after 5 seconds
                setTimeout(connect, 5000);
            };
            
            ws.onmessage = function(event) {
                const data = JSON.parse(event.data);
                
                if (data.type === 'welcome') {
                    console.log('Connected to query stream');
                    return;
                }
                
                // Display query
                displayQuery(data);
                updateStats(data);
            };
        }
        
        function displayQuery(query) {
            const queriesDiv = document.getElementById('queries');
            const div = document.createElement('div');
            
            const isSlowQuery = query.execution_time_ms > 1000;
            div.className = 'query-entry ' + (isSlowQuery ? 'slow-query' : 'fast-query');
            
            div.innerHTML = `
                <div class="query-meta">
                    <strong>${query.timestamp}</strong> - 
                    User: <strong>${query.user}</strong> - 
                    Module: <strong>${query.module}</strong> - 
                    Duration: <strong>${query.execution_time_ms}ms</strong>
                </div>
                <div class="query-sql">${query.query}</div>
            `;
            
            queriesDiv.insertBefore(div, queriesDiv.firstChild);
            
            // Keep only last 100 queries
            while (queriesDiv.children.length > 100) {
                queriesDiv.removeChild(queriesDiv.lastChild);
            }
        }
        
        function updateStats(query) {
            queryCount++;
            totalTime += query.execution_time_ms || 0;
            
            document.getElementById('queryCount').textContent = queryCount;
            document.getElementById('avgTime').textContent = Math.round(totalTime / queryCount) + 'ms';
        }
        
        function applyFilters() {
            if (!ws || ws.readyState !== WebSocket.OPEN) {
                alert('Not connected to query stream');
                return;
            }
            
            const filter = {
                user: document.getElementById('userFilter').value,
                query: document.getElementById('queryFilter').value,
                min_duration: parseFloat(document.getElementById('durationFilter').value) || 0
            };
            
            ws.send(JSON.stringify({
                type: 'filter',
                filter: filter
            }));
        }
        
        function clearFilters() {
            document.getElementById('userFilter').value = '';
            document.getElementById('queryFilter').value = '';
            document.getElementById('durationFilter').value = '';
            applyFilters();
        }
        
        // Connect on page load
        connect();
        
        // Apply filters on Enter key
        document.addEventListener('keypress', function(e) {
            if (e.key === 'Enter') {
                applyFilters();
            }
        });
    </script>
</body>
</html>
```

### 5. ELK Stack Integration for Historical Analysis

**Logstash configuration for query log processing:**

```ruby
# logstash-query-analysis.conf
input {
  file {
    path => "/path/to/erp_queries.jsonl"
    start_position => "end"
    codec => "json"
  }
}

filter {
  # Parse timestamp
  date {
    match => [ "timestamp", "ISO8601" ]
  }
  
  # Extract query patterns
  grok {
    match => { "query" => "(?<query_type>SELECT|INSERT|UPDATE|DELETE)" }
    tag_on_failure => ["_grokparsefailure_query_type"]
  }
  
  # Categorize query complexity
  if [query] =~ /JOIN/ {
    mutate { add_field => { "query_complexity" => "complex" } }
  } else if [query] =~ /(GROUP BY|ORDER BY|HAVING)/ {
    mutate { add_field => { "query_complexity" => "medium" } }
  } else {
    mutate { add_field => { "query_complexity" => "simple" } }
  }
  
  # Categorize by primary table
  if [query] =~ /\bproduto\b/i {
    mutate { add_field => { "primary_table" => "produto" } }
  } else if [query] =~ /\bcliente\b/i {
    mutate { add_field => { "primary_table" => "cliente" } }
  } else if [query] =~ /\bfornecedor\b/i {
    mutate { add_field => { "primary_table" => "fornecedor" } }
  } else if [query] =~ /\bvenda\b/i {
    mutate { add_field => { "primary_table" => "venda" } }
  } else if [query] =~ /\bestoque\b/i {
    mutate { add_field => { "primary_table" => "estoque" } }
  }
  
  # Performance categorization
  if [execution_time_ms] {
    if [execution_time_ms] < 100 {
      mutate { add_field => { "performance_category" => "fast" } }
    } else if [execution_time_ms] < 1000 {
      mutate { add_field => { "performance_category" => "moderate" } }
    } else {
      mutate { add_field => { "performance_category" => "slow" } }
    }
  }
  
  # Extract business operation context
  if [module] == "SearchDialog" {
    mutate { add_field => { "business_operation" => "search" } }
  } else if [module] =~ /Cadastro/ {
    mutate { add_field => { "business_operation" => "crud" } }
  } else if [module] =~ /Venda|Orcamento/ {
    mutate { add_field => { "business_operation" => "sales" } }
  }
}

output {
  elasticsearch {
    hosts => ["localhost:9200"]
    index => "erp-queries-%{+YYYY.MM.dd}"
  }
  
  # Also output to stdout for debugging
  stdout { codec => rubydebug }
}
```

**Kibana Dashboard Queries and Visualizations:**

```json
{
  "kibana_queries": {
    "slow_queries_by_user": {
      "query": "performance_category:slow AND execution_time_ms:>1000",
      "visualization": "data_table",
      "fields": ["user", "query", "execution_time_ms", "timestamp"]
    },
    "most_active_users": {
      "query": "*",
      "visualization": "pie_chart",
      "aggregation": {
        "field": "user.keyword",
        "size": 10
      }
    },
    "query_volume_timeline": {
      "query": "*",
      "visualization": "line_chart",
      "time_field": "@timestamp",
      "interval": "1m"
    },
    "table_access_patterns": {
      "query": "*",
      "visualization": "heat_map",
      "aggregation": {
        "x_axis": "primary_table.keyword",
        "y_axis": "user.keyword"
      }
    },
    "performance_distribution": {
      "query": "*",
      "visualization": "histogram",
      "field": "execution_time_ms",
      "interval": 100
    },
    "module_usage_analysis": {
      "query": "*",
      "visualization": "bar_chart",
      "aggregation": {
        "field": "module.keyword",
        "order": "desc"
      }
    }
  }
}
```

### 6. Command-Line Analysis Tools

**Bash scripts for quick query analysis:**

```bash
#!/bin/bash
# query_analysis.sh - Quick query log analysis tools

# Function to filter by user
filter_by_user() {
    local user=$1
    cat erp_queries.jsonl | jq -r "select(.user == \"$user\")"
}

# Function to find slow queries
find_slow_queries() {
    local threshold=${1:-1000}
    cat erp_queries.jsonl | jq -r "select(.execution_time_ms > $threshold) | {user, query, execution_time_ms, timestamp}"
}

# Function to get top queries by execution count
top_queries() {
    local limit=${1:-10}
    cat erp_queries.jsonl | jq -r '.query' | sort | uniq -c | sort -rn | head -$limit
}

# Function to get user activity summary
user_activity() {
    cat erp_queries.jsonl | jq -r '.user' | sort | uniq -c | sort -rn
}

# Function to monitor real-time queries for specific user
monitor_user() {
    local user=$1
    tail -f erp_queries.jsonl | jq -r "select(.user == \"$user\") | {timestamp, query, execution_time_ms}"
}

# Function to find queries affecting specific table
queries_for_table() {
    local table=$1
    cat erp_queries.jsonl | jq -r "select(.query | test(\"\\\\b$table\\\\b\"; \"i\")) | {user, query, execution_time_ms}"
}

# Usage examples
case "$1" in
    "user")
        filter_by_user "$2"
        ;;
    "slow")
        find_slow_queries "$2"
        ;;
    "top")
        top_queries "$2"
        ;;
    "activity")
        user_activity
        ;;
    "monitor")
        monitor_user "$2"
        ;;
    "table")
        queries_for_table "$2"
        ;;
    *)
        echo "Usage: $0 {user|slow|top|activity|monitor|table} [parameter]"
        echo "Examples:"
        echo "  $0 user joao.silva"
        echo "  $0 slow 500"
        echo "  $0 top 20"
        echo "  $0 monitor joao.silva"
        echo "  $0 table produto"
        ;;
esac
```

## Implementation Strategy

### Phase 1: Immediate Implementation (This Week)

**1. Application-Level Logging Setup:**
```cpp
// Add to your main.cpp or Application constructor
QueryLogger *queryLogger = new QueryLogger();
queryLogger->setLogFile("erp_queries.jsonl");
SqlQuery::setLogger(queryLogger);

// In user login handler
void Application::onUserLoggedIn(const QString &username) {
    queryLogger->setContext(username, "ERP");
}
```

**2. Basic Command-Line Analysis:**
```bash
# Create analysis script
chmod +x query_analysis.sh

# Start analyzing immediately
./query_analysis.sh activity  # See user activity
./query_analysis.sh slow 1000  # Find queries > 1 second
```

### Phase 2: Real-time Monitoring (Next Week)

**1. WebSocket Server Setup:**
```cpp
// Add to Application class
QueryStreamServer *streamServer = new QueryStreamServer(queryLogger);
queryLogger->enableRealTime(true);
```

**2. Launch Web Dashboard:**
- Save HTML dashboard as `query_monitor.html`
- Open in browser: `file:///path/to/query_monitor.html`
- Real-time filtering by user, query type, duration

### Phase 3: Advanced Analysis (Following Week)

**1. ELK Stack Integration:**
```bash
# Install ELK stack via Docker
docker-compose up -d elasticsearch kibana logstash

# Configure Logstash with provided config
# Import Kibana dashboard templates
```

**2. Performance Schema Integration:**
```cpp
// Add Performance Schema monitoring
PerformanceSchemaMonitor *psMonitor = new PerformanceSchemaMonitor("erp_user");
connect(psMonitor, &PerformanceSchemaMonitor::queryDetected, 
        this, &Application::handlePerformanceQuery);
```

## Debugging Workflows

### User-Specific Issue Investigation

```bash
# 1. Check recent activity for problematic user
./query_analysis.sh user "joao.silva" | tail -20

# 2. Monitor real-time for user
./query_analysis.sh monitor "joao.silva"

# 3. Find slow queries from user
cat erp_queries.jsonl | jq 'select(.user == "joao.silva" and .execution_time_ms > 500)'

# 4. Check what modules user is accessing
cat erp_queries.jsonl | jq -r 'select(.user == "joao.silva") | .module' | sort | uniq -c
```

### Performance Issue Investigation

```bash
# Find all slow queries
./query_analysis.sh slow 1000

# Check which tables are involved in slow queries
cat erp_queries.jsonl | jq -r 'select(.execution_time_ms > 1000) | .query' | grep -oiE '\b(produto|cliente|fornecedor|venda|estoque)\b' | sort | uniq -c

# Find users generating most slow queries
cat erp_queries.jsonl | jq -r 'select(.execution_time_ms > 1000) | .user' | sort | uniq -c | sort -rn
```

### Module-Specific Analysis

```bash
# Analyze SearchDialog performance
cat erp_queries.jsonl | jq 'select(.module == "SearchDialog")' | jq '.execution_time_ms' | sort -n | tail -10

# Check most common search patterns
cat erp_queries.jsonl | jq -r 'select(.module == "SearchDialog") | .query' | sort | uniq -c | sort -rn | head -10
```

## Benefits Over MySQL general_log

### ✅ Immediate Advantages
- **User Context**: Know exactly which application user executed each query
- **Structured Format**: JSON logs easy to parse and filter
- **Real-time Capability**: Live monitoring with WebSocket streaming
- **Module Context**: Understand which part of application generated queries
- **Performance Metrics**: Execution time tracking built-in
- **Minimal Performance Impact**: Much lower overhead than general_log

### ✅ Advanced Capabilities
- **Custom Filtering**: Filter by user, module, table, performance
- **Historical Analysis**: Trend analysis and pattern recognition
- **Business Intelligence**: Correlate queries with business operations
- **Debugging Power**: Trace specific user issues in real-time
- **Scalable Storage**: Can handle high-volume logging efficiently
- **Integration Ready**: Works with ELK stack, monitoring systems

### ✅ Operational Benefits
- **Proactive Monitoring**: Identify issues before users complain
- **Capacity Planning**: Understand usage patterns and growth
- **Security Auditing**: Track data access patterns
- **Performance Optimization**: Identify and fix slow queries
- **User Training**: Understand how users interact with system
- **Development Insights**: Guide feature development based on actual usage

## Conclusion

**Application-level structured logging** provides immediate relief from general_log limitations while building toward enterprise-grade query monitoring. The solution scales from simple file-based logging to real-time dashboards and comprehensive analytics.

**Key Success Factors:**
1. **Start Simple**: Begin with QueryLogger class integration
2. **Add Context Gradually**: Enhance with user and module context
3. **Build Real-time Capability**: Add WebSocket streaming for live debugging
4. **Scale to Analytics**: Integrate with ELK stack for advanced analysis

This approach transforms query monitoring from a debugging afterthought into a powerful operational and development tool, providing the user-specific, real-time visibility you need for effective ERP system management.