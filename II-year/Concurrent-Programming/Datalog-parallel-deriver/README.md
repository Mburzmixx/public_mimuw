# Datalog parallel deriver
**My code is located in** `Datalog-parallel-deriver/solv/src/main/engine/ParallelDeriver.java`.

All other files were provided by the course coordinators as starter code and documentation.

## Project overview
This is a concurrent-programming assignment that extends a Datalog engine. The project includes a generated parser, a single-threaded reference engine, a pluggable oracle interface, and a test suite with example Datalog programs and expected results.

## Scope of my contribution
I implemented the parallel derivation logic in `ParallelDeriver.java`. Everything else in the archive (parser, tests, examples, build configuration, and helper utilities) was provided as starter code and documentation by the course coordinators.

In order to test execute:

`cd ./Datalog-parallel-deriver/solv`

`make test`

## Archive contents
The assignment includes an archive with the parser code and the single-threaded engine. The parser and tests require external libraries, so the project uses the Maven build system. Opening the project in an IDE should make building and running straightforward (for example, VS Code with the Extension Pack for Java). Alternatively, you can use the included Makefile from the console. The project follows the Maven Standard Directory Layout.

The source code consists of:

- `src/main/java/cp2025/datalog/`: automatically generated parser code.
	- It can be regenerated from the grammar defined in `/Datalog.cf` using `make datalog`.
- `src/main/java/cp2025/engine/`:
	- `Datalog.java`: basic structures describing a Datalog program (atoms, rules, etc.), with helper functions:
		- `toString`
		- `Datalog.Program.validate()`
		- `List<Variable> getVariables(List<Atom>)`
	- `Parser.java`: wrapper for the generated parser with static methods that accept program code (file path, open `Reader`, or `String`) and return a validated `Datalog.Program`.
	- `AbstractDeriver.java` and `AbstractOracle.java`: engine and oracle interfaces.
	- `NullOracle.java`: a trivial oracle (no predicate is computable).
	- `Main.java`: usage example.
	- `SimpleDeriver.java`: the single-threaded engine described above.
	- `ParallelDeriver.java`: empty template to fill in.
	- `FunctionGenerator.java`: helper class for iterating over all assignments.
	- `Unifier.java`: static helper methods for unifying a rule with a goal (the derived statement).
- `test/java/cp2025/`: sample basic tests (passing them does not guarantee any points).

Additionally, the archive contains:

- `examples/`: examples of Datalog programs with results (using `NullOracle`).
- `lib/`: dependencies.
- `.vscode/java-formatter.xml`: automatic code formatting configuration (Ctrl+Shift+I in VS Code).
- `Makefile`: makes it easier to use Maven from the console.
- `mvnw`, `mvnw.cmd`: Maven Wrapper executables that should allow building the project on any system (we still recommend working in the students environment).
- `pom.xml`: Maven project specification (Java version, dependencies).
- `bnfc`: BNFC executable for generating parsers.
