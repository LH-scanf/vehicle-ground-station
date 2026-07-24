# Codex Development Instructions

## Project

This repository contains a Windows vehicle ground station built with:

- C++20
- Qt 6
- Qt Quick / QML
- CMake
- Qt WebSockets

The application communicates with a Linux ROS2 vehicle gateway through WebSocket and JSON.

The Windows application itself must not directly depend on ROS2.

Read the relevant documents under `docs/` before making architectural, protocol, or requirement changes.

### Related ROS2 repository

The Linux vehicle code and ROS2 integration live in the separate repository:

```text
https://github.com/LH-scanf/ros-car.git
```

- Treat its `main` branch as the current ROS2 Humble baseline and its `jazzy` branch as the ROS2 Jazzy baseline.
- Develop and integrate shared gateway behavior against Humble first while that test environment is available, then merge the same distro-neutral gateway changes into `jazzy` and verify compatibility there.
- Implement the ROS2-facing `vehicle_gateway` in that repository, not in this Windows Qt repository.
- Keep this repository limited to the WebSocket client, protocol validation, application state, and operator interface.
- Before changing a ROS2 topic, message type, service, action, or safety behavior, inspect the corresponding implementation in `ros-car` and keep `docs/protocol.md` synchronized.
- Do not copy ROS2 packages or Python ROS2 nodes into this repository.

------

## Project goals

The current goal is to build a Windows vehicle ground station for a single ROS2 vehicle.

The ground station will eventually support:

- Vehicle connection and status monitoring
- Two-dimensional SLAM map display
- Vehicle position and heading display
- Navigation target selection
- Vehicle control mode switching
- Emergency stop
- Mission status display
- Diagnostics and log recording

Foxglove and RViz remain development and debugging tools. They are not the final operator interface.

------

## Technology stack

Use the following technologies unless the user explicitly approves a change:

- C++20
- Qt 6
- Qt Quick / QML
- CMake
- Qt WebSockets
- JSON
- SQLite when local persistent storage is required
- Qt Test for automated tests

Do not introduce another UI framework, network framework, database, or third-party library without explaining:

1. What problem it solves
2. Why Qt's built-in functionality is insufficient
3. What deployment or maintenance cost it introduces

------

## Development rules

1. Do not implement the entire requirements document at once.

2. Only implement the task explicitly requested by the user.

3. Prefer a small, runnable vertical slice over multiple empty modules.

4. Keep the project buildable after every task.

5. Keep business logic, communication, protocol parsing, state management, and data processing in C++.

6. Keep page layout, visual presentation, animations, and basic user interaction in QML.

7. Do not put WebSocket protocol parsing or complex business logic directly in QML.

8. Do not introduce ROS2 dependencies into the Windows application.

9. Do not modify the communication protocol without updating:

   `docs/protocol.md`

10. Do not change established architecture or directory responsibilities without updating:

    `docs/architecture.md`

11. Add explanatory comments for:

    - World, map, grid, and screen coordinate conversion
    - WebSocket and JSON protocol parsing
    - Timeout and reconnect behavior
    - Safety-related control logic
    - Non-obvious mathematical operations

12. Avoid comments that merely repeat the code.

13. Do not create empty classes or placeholder abstractions unless they are required by the current task.

14. Do not perform unrelated refactoring while implementing a focused task.

15. Do not silently rename protocol fields, QML properties, public classes, or configuration keys.

16. When a breaking change is necessary, explain the impact before applying it.

17. Important vehicle commands must distinguish between:

    - Command sent
    - Command acknowledged
    - Command successfully executed
    - Command failed or timed out

18. Safety behavior must not depend only on the Windows interface. Communication-loss protection must also exist on the vehicle side.

------

## Multi-computer development rules

This repository is developed on more than one Windows computer.

All code and configuration must remain portable between development machines.

### Portable paths

1. Do not commit machine-specific absolute paths.

2. Do not write paths such as:

   ```text
   C:\Users\username\Qt\6.8.0\msvc2022_64
   D:\Projects\vehicle_ground_station
   ```

   into source code, committed CMake files, QML files, or shared configuration files.

3. Use relative paths based on the repository root whenever possible.

4. Qt installation paths and compiler locations must be provided through the local development environment, CMake presets, IDE settings, or environment variables.

5. Do not assume both computers use the same:

   - Qt installation directory
   - Compiler
   - CMake generator
   - Drive letter
   - Username
   - Build directory
   - IDE

### Local and shared configuration

Shared default configuration belongs in:

```text
config/default_config.json
```

Machine-specific or developer-specific configuration should use a local file such as:

```text
config/local_config.json
```

The local configuration file must not be committed.

The application should:

1. Load shared defaults first.
2. Load local overrides if the local file exists.
3. Continue running when the local override file is absent.

Do not store secrets, personal paths, temporary IP addresses, or machine-specific settings in committed files.

### Build directories

Build output must remain outside tracked source files.

Recommended local build directories include:

```text
build/
build-debug/
build-release/
out/
```

These directories must be ignored by Git.

Do not commit:

- Compiled executable files
- Object files
- Generated QML cache files
- CMake cache files
- IDE build output
- Temporary logs
- Local databases
- User-specific IDE settings

### Consistent source files

Use:

- UTF-8 encoding
- Consistent line endings
- Consistent file naming
- Consistent CMake target names

Do not allow one computer to repeatedly rewrite files only because of different encoding or line-ending settings.

A `.gitattributes` file should define text normalization for source files.

### Dependency consistency

Both computers should use compatible major and minor Qt versions whenever possible.

The required development environment must be documented in:

```text
docs/development_setup.md
```

That document should record:

- Supported Qt version
- Supported compiler
- CMake minimum version
- Required Qt modules
- Configuration commands
- Build commands
- Run instructions
- Known environment differences

Do not hard-code a specific computer's setup into project source files.

------

## Git workflow

1. Pull the latest changes before starting a new task on either computer.
2. Do not begin work on stale files when the same area may have changed on the other computer.
3. Keep commits focused on one logical change.
4. Do not mix generated files, formatting-only changes, and functional changes in the same commit unless necessary.
5. Before committing:
   - Build the project
   - Review changed files
   - Remove accidental generated files
   - Confirm that no local paths or secrets were added
6. Push completed work before switching to the other computer.
7. Do not use force push unless the user explicitly requests it and understands the impact.
8. Do not overwrite unresolved changes from another computer.
9. When merge conflicts occur, preserve the intended behavior from both sides rather than blindly choosing one version.
10. Do not commit directly generated build output to solve an environment problem.

------

## Directory responsibilities

Use the following directory responsibilities:

```text
vehicle_ground_station/
├── AGENTS.md
├── CMakeLists.txt
├── README.md
├── docs/
├── config/
├── resources/
├── src/
├── qml/
└── tests/
```

### `src/`

Contains C++ implementation, including:

- Application controllers
- Communication
- Protocol parsing
- Vehicle state
- Mission management
- Map processing
- Alarm handling
- Logging
- Configuration management

### `qml/`

Contains:

- Application window
- Pages
- Reusable visual components
- Visual themes
- QML resources

Do not place network parsing or persistent storage logic in this directory.

### `docs/`

Contains:

- `requirements.md`
- `architecture.md`
- `protocol.md`
- `development_setup.md`
- Task documents under `docs/tasks/`

### `config/`

Contains shared default configuration.

Machine-specific configuration must not be committed.

### `tests/`

Contains automated tests for C++ business logic, protocol parsing, coordinate conversion, and other testable components.

### `resources/`

Contains icons, images, fonts referenced by the application, and Qt resource definitions.

Do not store generated build resources in this directory.

------

## Current development stage

The project is currently in V0.1.

The first objective is to create a runnable Qt application containing:

- Main window
- Top status bar
- Navigation sidebar
- Dashboard page
- Map placeholder page
- Diagnostics placeholder page
- Settings page
- A C++ vehicle state object exposed to QML
- Mock vehicle data

Use mock vehicle data until WebSocket communication is implemented.

Do not implement real ROS2 integration, map parsing, mission execution, SQLite storage, or full WebSocket communication unless the current task explicitly requests it.

------

## Initial UI scope

The initial application should contain the following pages:

### Dashboard

Displays mock values for:

- Connection status
- Vehicle mode
- Speed
- Battery percentage
- GPS status
- Emergency stop status

### Map

Initially contains only a clearly marked placeholder.

Do not implement a real map until the map interface and coordinate rules are defined.

### Diagnostics

Initially contains a basic log or diagnostic placeholder.

### Settings

Initially contains basic non-functional placeholders for connection settings.

Do not persist settings until configuration behavior is explicitly requested.

------

## Architecture boundaries

Use the following basic responsibility split:

```text
QML interface
    ↓
C++ application and state layer
    ↓
C++ communication and protocol layer
    ↓
WebSocket + JSON
    ↓
Linux vehicle gateway
    ↓
ROS2
```

QML must not communicate directly with ROS2.

QML should not directly own the authoritative vehicle state.

The C++ state layer should expose data to QML using suitable Qt mechanisms such as:

- `QObject`
- `Q_PROPERTY`
- Signals and slots
- `QAbstractListModel` when list data is required

Avoid global mutable state.

------

## Communication rules

When WebSocket communication is introduced:

1. Keep WebSocket ownership in C++.
2. Parse incoming JSON in C++.
3. Validate message type and required fields.
4. Handle malformed messages without crashing.
5. Update the vehicle state through a clear state-management interface.
6. Send command acknowledgements and timeout results to the UI.
7. Do not treat a successfully transmitted command as a successfully executed command.
8. Keep protocol field names consistent with `docs/protocol.md`.
9. Log connection, disconnection, reconnect, parse failures, commands, and acknowledgements.
10. Avoid blocking the UI thread.

The existing Foxglove bridge is a reference for currently available telemetry fields, but the independent Qt ground station should use the project's own documented WebSocket and JSON protocol rather than implementing the Foxglove WebSocket protocol.

------

## Safety rules

When vehicle control is implemented:

1. Manual movement commands are allowed only when the vehicle reports that it is in ground-control mode.
2. The UI must use the actual mode reported by the vehicle, not only the locally requested mode.
3. Emergency stop must remain visible and accessible from primary operating pages.
4. Emergency stop activation must disable ordinary movement and mission-start controls.
5. Emergency stop must never be cleared automatically.
6. Loss of connection must disable controls in the UI.
7. The vehicle side must independently stop or enter a safe state when control messages time out.
8. Safety timeout values must be configurable and documented.
9. Safety-related changes require tests or a documented manual verification procedure.

------

## Build and validation

The exact local Qt path may differ between computers.

Do not commit commands containing a developer-specific Qt installation path.

Typical configuration and build commands are:

```powershell
cmake -S . -B build
cmake --build build --config Debug
```

When required, the developer may locally provide `CMAKE_PREFIX_PATH`, select a Qt Kit in Qt Creator, or use a local CMake preset.

After modifying C++ code, QML files, CMake files, or Qt resources:

1. Configure the project if needed.
2. Build the project.
3. Report whether the build succeeded.
4. If the build cannot be run because the local environment is unavailable, state that clearly.
5. Do not claim successful compilation without actually running the build.

When a task changes UI behavior, also report the expected manual verification steps.

------

## Definition of done

A task is complete only when:

1. The requested behavior is implemented.
2. The implementation stays within the requested scope.
3. The project builds successfully, or the inability to build is clearly reported.
4. Existing completed behavior is not intentionally broken.
5. Relevant tests are added or updated when appropriate.
6. Relevant documentation is updated.
7. No machine-specific absolute paths are committed.
8. No generated build files, secrets, temporary logs, or local configuration files are committed.
9. The final response lists:
   - Files created
   - Files modified
   - Main implementation decisions
   - Build or test commands executed
   - Build and test results
   - Remaining limitations

------

## Task execution process

Before modifying code:

1. Read this `AGENTS.md`.
2. Read the current task document.
3. Read only the project documents relevant to the task.
4. Inspect the existing repository structure.
5. Briefly state which files are expected to be created or modified.

During implementation:

1. Keep changes focused.
2. Preserve buildability.
3. Avoid unrelated refactoring.
4. Do not expand scope without user approval.

After implementation:

1. Review the diff.
2. Build the project.
3. Run relevant tests.
4. Check for machine-specific paths and generated files.
5. Update documentation when required.
6. Summarize the result and any remaining issues.

------

## Task priority

When requirements conflict, follow this priority:

1. Explicit instructions in the current user task
2. Safety requirements
3. The current task document under `docs/tasks/`
4. `docs/protocol.md`
5. `docs/architecture.md`
6. `docs/requirements.md`
7. This file's general development conventions

Do not silently resolve a major conflict. Explain the conflict before making a breaking or architectural decision.
