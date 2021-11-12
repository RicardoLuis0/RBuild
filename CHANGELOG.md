### 0.0.0g
* add Target Groups, see [`FORMAT.md`](FORMAT.md) for more info

### 0.0.0f
* `-clean`, remove build data for specified targets

### 0.0.0e
* `-static`, force static linking, might not work for every project (possible link/runtime errors)

### 0.0.0d
* `-num_jobs=auto`, run as many jobs as there are CPUs in the system

### 0.0.0c
* `-incremental_build_exclude_system` or `-MMD`, use `-MMD` instead of `-MD` for calculating dependencies
* cache file write times when resolving incremental builds, disable with `-filetime_nocache`

### 0.0.0b
* allow `"generic"` as a driver, requires user-provided executable

### 0.0.0a
* First versioned build
