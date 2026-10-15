#pragma once

class SPDBConnection;

/// Abstract base for all ORM model classes.
/// Each concrete subclass maps to one database table and is responsible for
/// serialising itself to/from the database via an SPDBConnection.
class SPModel {
public:
    virtual ~SPModel() = default;

    /// Persist this model (INSERT or UPDATE). Returns true on success.
    virtual bool save(SPDBConnection& conn) = 0;

    /// Delete this model from the database. Returns true on success.
    virtual bool remove(SPDBConnection& conn) = 0;

    /// True when the model has a valid primary key (has been persisted at
    /// least once or was loaded from the database).
    virtual bool isPersisted() const = 0;

    SPModel()                            = default;
    SPModel(const SPModel&)              = default;
    SPModel& operator=(const SPModel&)   = default;
};