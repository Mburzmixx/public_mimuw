// Implemented by Mateusz Burza, mb469200
package cp2025.engine;

import java.util.*;
import java.util.concurrent.ConcurrentHashMap;
import java.util.concurrent.ConcurrentLinkedQueue;

import cp2025.engine.Datalog.*;



public class ParallelDeriver implements AbstractDeriver {    
    /* `numWorkers` - number of worker threads to be used for derivation.
     * `globalCancel` - flag indicating if the entire derivation process should be cancelled.
     */
    private final int numWorkers;
    private static volatile boolean globalCancel = false;

    /* `results` - map of queries to their derivability results.
     * `predicateToRules` - map of predicates to rules that can derive them.
     */
    private static final ConcurrentHashMap<Atom, Boolean> results = new ConcurrentHashMap<>();
    private static Map<Predicate, List<Rule>> predicateToRules;

    /* `queryQueue` - queue of queries to be processed by workers.
     */
    private static final ConcurrentLinkedQueue<Atom> queryQueue = new ConcurrentLinkedQueue<>();

    /* `knownStatements` - map of already derived statements (true/false).
     * `derivingWorkers` - map of statements to threads currently deriving them.
     *                          (I declare Set as ConcurrentHashMap.newKeySet() to avoid synchronization issues)
     */
    private static final ConcurrentHashMap<Atom, Boolean> knownStatements = new ConcurrentHashMap<>();
    private static final ConcurrentHashMap<Atom, Set<Thread>> derivingWorkers = new ConcurrentHashMap<>();

    

    /* Exception used to signal that the current derivation should be cancelled,
     * it propagates until finds `goal` on worker's stack
     */
    private static class LocalCancellationException extends RuntimeException {
        public final Atom computedAtom;
        public LocalCancellationException(Atom computedAtom) {
            // stackless: without stack trace and suppression
            super(null, null, false, false);
            this.computedAtom = computedAtom;
        }
        @Override
        public synchronized Throwable fillInStackTrace() { return this; }

        public Boolean isLookingFor(Atom atom) {
            return this.computedAtom.equals(atom);
        }
    }

    private static class Worker implements Runnable {
        private final Program input;
        private final AbstractOracle oracle;

        /* `inProgressStatements` - set of statements currently being derived by this worker.
         * `statementsToDerive` - stack (list) of statements being derived by this worker (in order).
         */
        private final Set<Atom> inProgressStatements = new HashSet<>();
        private final List<Atom> statementsToDerive = new ArrayList<>();
        
        private record DerivationResult(boolean derivable, Set<Atom> failedStatements) {}


        public Worker(Program input, AbstractOracle oracle) {
            this.input = input;
            this.oracle = oracle;
        }

        /* Function in which each worker takes queries from the shared queue and tries to derive them. */
        @Override
        public void run() {
            Atom goal;
            try {
                while ((goal = queryQueue.poll()) != null) {
                    DerivationResult result = deriveStatement(goal);

                    results.put(goal, result.derivable);
                    
                    handleIfInterrupted();
                }
            } catch (InterruptedException e) {
                clearStructures();
                return;
            }   
        }

        /* Main function responsible for deriving a single statement.
         * 1. Checks if the statement is already known.
         * 2. Tries to derive it using the oracle. (and checks calculatability)
         * 3. If not derivable yet, tries to derive it using rules.
         * 4. Updates internal data structures and notifies other workers as needed.
         *
         * Regularly checks for interruptions and handles them.
         * 
         * This is the function, in which `LocalCancellationExceptions` are handled.
         * This process is as follows:
         *   - If the exception is for the current goal, returns the known result.
         *   - Otherwise, rethrows the exception to be handled by the caller.
         * 
         * Function `handleIfInterrupted()` throws `LocalCancellationException`
         * if and only if the current thread has on its derivation stack a statement
         * that has already been computed by another thread.
         * (i.e. `LocalCancellationException` will not stop the program entirely)
         */
        private DerivationResult deriveStatement(Atom goal) throws InterruptedException {
            try {

                Boolean hasBeenDerived = knownStatements.get(goal);
                if (hasBeenDerived != null)
                    return new DerivationResult(hasBeenDerived, Set.of());

                if (inProgressStatements.contains(goal))
                    // Return false but do not store the result (we may find a different derivation later).
                    return new DerivationResult(false, Set.of(goal));

                inProgressStatements.add(goal);
                addGoal(goal);
                
                Boolean hasBeenDerived_check = knownStatements.get(goal);
                if (hasBeenDerived_check != null) {
                    inProgressStatements.remove(goal);
                    cleanupGoal(goal);

                    return new DerivationResult(hasBeenDerived_check, Set.of());
                }

                handleIfInterrupted();

                // Try to derive the statement using the oracle.
                try {
                    if (oracle.isCalculatable(goal.predicate())) {
                        boolean result = oracle.calculate(goal);
                        knownStatements.put(goal, result);

                        inProgressStatements.remove(goal);
                        cleanupGoal(goal);
                        notifyDerivingWorkers(goal);

                        return new DerivationResult(result, Set.of());
                    }
                } catch (InterruptedException e) {
                    // Here I pass also flag=true, sice `InterruptedException` means 
                    // that the thread was interrupted during execution of `calculate()`.
                    handleIfInterrupted(e, true);
                }

                // Try to derive the statement using rules.
                DerivationResult result = deriveNewStatement(goal);

                inProgressStatements.remove(goal);
                cleanupGoal(goal);

                if (result.derivable) {
                    knownStatements.put(goal, true);
                    notifyDerivingWorkers(goal);

                } else if (inProgressStatements.isEmpty()) {
                    for (Atom s : result.failedStatements) {
                        // Only mark as false if no other thread found it derivable
                        knownStatements.put(s, false);
                        notifyDerivingWorkers(s);
                    }
                }

                return result;
            } catch (LocalCancellationException le) {
                cleanupGoal(goal);
                inProgressStatements.remove(goal);

                if (le.isLookingFor(goal))
                    return new DerivationResult(knownStatements.get(goal), Set.of());
                else
                    throw le;
            }
        }

        /* Same as in `SimpleDeriver` (only with some lines `handleIfInterrupted()`).
         * Tries to unify the goal with each rule. */
        private DerivationResult deriveNewStatement(Atom goal) throws InterruptedException {
            List<Rule> rules = predicateToRules.get(goal.predicate());
            if (rules == null)
                return new DerivationResult(false, Set.of(goal));

            Set<Atom> failedStatements = new HashSet<>();

            for (Rule rule : rules) {
                handleIfInterrupted();

                Optional<List<Atom>> partiallyAssignedBody = Unifier.unify(rule, goal);
                if (partiallyAssignedBody.isEmpty())
                    continue;

                List<Variable> variables = Datalog.getVariables(partiallyAssignedBody.get());
                FunctionGenerator<Variable, Constant> iterator = new FunctionGenerator<>(variables,
                        input.constants());
                for (Map<Variable, Constant> assignment : iterator) {
                    handleIfInterrupted();

                    List<Atom> assignedBody = Unifier.applyAssignment(partiallyAssignedBody.get(),
                            assignment);
                    DerivationResult result = deriveBody(assignedBody);
                    if (result.derivable) {
                        return new DerivationResult(true, Set.of());
                    }
                    failedStatements.addAll(result.failedStatements);
                }
            }

            failedStatements.add(goal);
            return new DerivationResult(false, failedStatements);
        }

        /* Same as in `SimpleDeriver`, derives all atoms in the body sequentially. */
        private DerivationResult deriveBody(List<Atom> body) throws InterruptedException {
            for (Atom statement : body) {
                DerivationResult result = deriveStatement(statement);
                if (!result.derivable)
                    return new DerivationResult(false, result.failedStatements);
            }
            return new DerivationResult(true, Set.of());
        }


        private void handleIfInterrupted() throws InterruptedException {
            handleIfInterrupted(null, false);
        }

        /* Function responsible for handling interruptions of this worker thread.
         * 1. If `globalCancel` is set, re-interrupts the thread and throws InterruptedException.
         * 2. Otherwise - it has been `LocalCancellationException`, so checks the thread's recursive `Atom` derivation stack.
         * 3. Throws appropriate exception.
         */
        private void handleIfInterrupted(InterruptedException e, boolean flag) throws InterruptedException {
            if (Thread.interrupted() || flag) {
                if (globalCancel) {
                    Thread.currentThread().interrupt();
                    throw e != null ? e : new InterruptedException("Derivation process was interrupted.");
                }

                for (Atom statement : statementsToDerive) {
                    if (knownStatements.containsKey(statement)) {
                        throw new LocalCancellationException(statement);
                    }
                }
            }
        }

        /* Function responsible for notifying all workers deriving the given statement.
         * While notifying, it also updates their checkpoints to the last known value.
         */
        private void notifyDerivingWorkers(Atom statement) {
            Set<Thread> threadsSet = derivingWorkers.get(statement);

            if (threadsSet != null) {
                for (Thread thread : threadsSet)
                    thread.interrupt();
            }
        }

        /* Function responsible for registering a goal as being derived by this worker. */
        private void addGoal(Atom goal) {
            derivingWorkers.computeIfAbsent(goal, k -> ConcurrentHashMap.newKeySet()).add(Thread.currentThread());
            statementsToDerive.add(goal);
        }

        /* Function responsible for cleaning up after a goal has been processed.
         * Following statements should hold:
         *   -> `derivingWorkers[goal]` is not null,
         *   -> `statementsToDerive`'s last element is goal.
         */
        private void cleanupGoal(Atom goal) {
            if (derivingWorkers.get(goal) != null)
                derivingWorkers.get(goal).remove(Thread.currentThread());

            if (statementsToDerive.size() > 0 && statementsToDerive.get(statementsToDerive.size() - 1).equals(goal))
                statementsToDerive.remove(statementsToDerive.size() - 1);
        }

        /* Function responsible for clearing all internal data structures. */
        private void clearStructures() {
            inProgressStatements.clear();
            statementsToDerive.clear();
        }
    }


    public ParallelDeriver(int numWorkers) { this.numWorkers = numWorkers; }

    /* Function responsible for deriving a program using multiple worker threads.
     * Creates `numWorkers` threads, each running a Worker instance.
     * Each worker retrieves queries from the shared `queryQueue` and attempts to derive them.
     * The results are stored in the shared `results` map.
     */
    @Override
    public Map<Atom, Boolean> derive(Program input, AbstractOracle oracle)
            throws InterruptedException {
        try {
            predicateToRules = input.rules().stream().collect(
                    java.util.stream.Collectors.groupingBy(rule -> rule.head().predicate()));
            
            input.queries().forEach(queryQueue::add);

            List<Thread> workers = new ArrayList<>();

            for (int i = 0; i < numWorkers; i++) {
                Thread workerThread = new Thread(new Worker(input, oracle));

                workers.add(workerThread);
                workerThread.start();
            }

            handleIfMainInterrupted(null, workers);

            try {
                for (Thread worker : workers) {
                    worker.join();
                    handleIfMainInterrupted(null, workers);
                }
            } catch (InterruptedException e) {
                handleIfMainInterrupted(e, workers);
            }

            Map<Atom, Boolean> out = new HashMap<>(results);

            return out;
        } finally {
            results.clear();
            predicateToRules = null;
            globalCancel = false;
            queryQueue.clear();
            knownStatements.clear();
            derivingWorkers.clear();
        }

    }

    /* Function responsible for handling interruptions of the main thread.
     * 1. Sets `globalCancel` to true.
     * 2. Interrupts all worker threads.
     * 3. Joins all worker threads.
     * 4. (Re)throws InterruptedException to notify the caller.
     */
    private void handleIfMainInterrupted(InterruptedException e, List<Thread> workers) throws InterruptedException {
        if (Thread.currentThread().isInterrupted() || e != null) {
            globalCancel = true;

            workers.forEach(Thread::interrupt);

            try {
                for (Thread worker : workers) {
                    worker.join();
                }
            } catch (InterruptedException ex) {
                throw ex;
            }
         
            throw e != null ? e : new InterruptedException("Derivation interrupted.");
        }
    }
}
