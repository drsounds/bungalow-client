package se.spacify.db;

import com.j256.ormlite.dao.BaseDaoImpl;
import com.j256.ormlite.dao.Dao;
import com.j256.ormlite.dao.DaoManager;
import com.j256.ormlite.dao.GenericRawResults;
import com.j256.ormlite.field.FieldType;
import com.j256.ormlite.jdbc.JdbcConnectionSource;
import com.j256.ormlite.support.ConnectionSource;
import com.j256.ormlite.table.TableInfo;
import com.j256.ormlite.table.TableUtils;

import java.nio.file.Files;
import java.nio.file.Path;
import java.sql.SQLException;
import java.util.HashSet;
import java.util.Locale;
import java.util.Map;
import java.util.Set;
import java.util.concurrent.ConcurrentHashMap;

/**
 * Singleton that owns the SQLite connection and hands out ORMLite DAOs.
 *
 * <p>It is deliberately ignorant of every concrete entity: apps and concepts own
 * their own entity classes (each in its app's {@code model} package) and register
 * their tables on activation via {@code ApplicationContext.registerEntity(...)} /
 * {@code ConceptContext.registerEntity(...)}, which delegate here. Tables are
 * created on demand and DAOs cached, so registration order between plugins does
 * not matter — reading an entity another plugin owns auto-ensures its table.
 *
 * <p>Call {@link #init()} once at startup (opens the connection only) and
 * {@link #close()} on shutdown.
 */
public class DatabaseManager {

    private static DatabaseManager instance;

    private ConnectionSource connectionSource;

    /** Per-entity DAO cache; the key's table is created on first access. */
    private final Map<Class<?>, Dao<?, ?>> daoCache = new ConcurrentHashMap<>();

    private DatabaseManager() {}

    public static DatabaseManager getInstance() {
        if (instance == null) instance = new DatabaseManager();
        return instance;
    }

    /** Open the SQLite connection. Does not create any tables — apps register their own. */
    public void init() throws Exception {
        Path dbFile = Path.of(System.getProperty("user.home"), ".spacify", "library.db");
        Files.createDirectories(dbFile.getParent());

        connectionSource = new JdbcConnectionSource("jdbc:sqlite:" + dbFile);
    }

    public void close() {
        if (connectionSource != null) {
            try { connectionSource.close(); } catch (Exception ignored) {}
            connectionSource = null;
        }
        daoCache.clear();
    }

    public ConnectionSource getConnectionSource() { return connectionSource; }

    // ── Entity registry ─────────────────────────────────────────────────────────

    /**
     * Ensure {@code entity}'s table exists and return its (cached) DAO. Idempotent
     * and order-independent: the table is created-if-not-exists on first call.
     */
    @SuppressWarnings("unchecked")
    public <T> Dao<T, Integer> dao(Class<T> entity) {
        return (Dao<T, Integer>) daoCache.computeIfAbsent(entity, c -> {
            try {
                TableUtils.createTableIfNotExists(connectionSource, c);
                Dao<?, ?> d = DaoManager.createDao(connectionSource, c);
                reconcileColumns(d);
                return d;
            } catch (SQLException e) {
                throw new RuntimeException("Failed to register entity " + c.getName(), e);
            }
        });
    }

    // ── Lightweight schema migration ──────────────────────────────────────────────

    /**
     * Bring an existing table up to its entity's current fields without dropping
     * data: {@code createTableIfNotExists} only creates missing tables, never
     * alters existing ones, so a table written by an older build can lack columns
     * the code now reads (e.g. the {@code name}/{@code version} columns the
     * {@code Node} base added). For each {@link com.j256.ormlite.field.DatabaseField}
     * column absent from the live table we {@code ALTER TABLE … ADD COLUMN} it
     * (nullable, so existing rows are fine), then backfill the one documented rename
     * — legacy {@code title} → {@code name}. New/fresh tables already match, so this
     * is a no-op for them.
     */
    private void reconcileColumns(Dao<?, ?> dao) {
        if (!(dao instanceof BaseDaoImpl<?, ?> impl)) return;
        TableInfo<?, ?> info = impl.getTableInfo();
        String table = info.getTableName();
        try {
            Set<String> existing = existingColumns(dao, table);
            if (existing.isEmpty()) return; // table not readable / just created empty — leave it

            boolean addedName = false;
            for (FieldType ft : info.getFieldTypes()) {
                // ForeignCollection fields (e.g. Playlist.rows) are not columns.
                if (ft.isForeignCollection()) continue;
                String col = ft.getColumnName();
                if (col == null || existing.contains(col.toLowerCase(Locale.ROOT))) continue;
                dao.executeRawNoArgs(
                    "ALTER TABLE `" + table + "` ADD COLUMN `" + col + "` " + sqlTypeFor(ft));
                if ("name".equalsIgnoreCase(col)) addedName = true;
            }

            // Legacy title→name rename: carry the old display value over.
            if (addedName && existing.contains("title")) {
                dao.executeRawNoArgs(
                    "UPDATE `" + table + "` SET `name` = `title` WHERE `name` IS NULL");
            }
        } catch (Exception e) {
            // Best-effort migration: never let a reconcile hiccup break activation.
            System.err.println("Schema reconcile skipped for " + table + ": " + e.getMessage());
        }
    }

    /** Lower-cased set of the columns currently present in {@code table}. */
    private Set<String> existingColumns(Dao<?, ?> dao, String table) throws SQLException {
        Set<String> cols = new HashSet<>();
        try (GenericRawResults<String[]> res =
                 dao.queryRaw("PRAGMA table_info(`" + table + "`)")) {
            for (String[] row : res) {
                // PRAGMA table_info columns: cid, name, type, notnull, dflt_value, pk
                if (row.length > 1 && row[1] != null) cols.add(row[1].toLowerCase(Locale.ROOT));
            }
        } catch (Exception e) {
            // TODO Auto-generated catch block
            e.printStackTrace();
        }
        return cols;
    }

    /**
     * A SQLite column type for a field. Derived from the Java field type (not
     * {@code ft.getSqlType()}, whose converter is null for some fields) — and SQLite
     * is dynamically typed anyway, so this only sets column affinity. Foreign keys
     * point at an {@code int} generated id, so they get {@code INTEGER}.
     */
    private static String sqlTypeFor(FieldType ft) {
        if (ft.isForeign()) return "INTEGER";
        Class<?> t = ft.getType();
        if (t == int.class  || t == Integer.class || t == boolean.class || t == Boolean.class
         || t == short.class || t == Short.class  || t == byte.class    || t == Byte.class) return "INTEGER";
        if (t == long.class || t == Long.class) return "BIGINT";
        if (t == float.class || t == Float.class || t == double.class || t == Double.class) return "DOUBLE";
        return "VARCHAR";
    }

    /**
     * Eagerly create an app's entity table at activation time. Same semantics as
     * {@link #dao(Class)}; named for use from {@code onActivate} registration.
     */
    public <T> Dao<T, Integer> registerEntity(Class<T> entity) {
        return dao(entity);
    }
}
