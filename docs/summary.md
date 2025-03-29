# Summary: Vehicle Routing Problem with Transshipments for Solid Waste Collection with Transfer Stations (VRPT-SWTS)

Based on Ghiani et al. (2021) and the provided assignment description.

## 1. Problem Description

The core problem is to optimize a Municipal Solid Waste (MSW) collection system that utilizes intermediate Solid Waste Transfer Stations (SWTS). This is motivated by landfills being located far from urban centers and access restrictions for large vehicles.

The system involves two types of vehicles:
1.  **Collection Vehicles (CVs):** Small vehicles that collect waste from various **collection points** (zones/clusters of citizens) and transport it to an SWTS.
2.  **Transportation Vehicles (TVs):** Large, long-haul vehicles that transport consolidated waste from SWTS to the final **landfill**.

The problem is modeled as a **Vehicle Routing Problem with Transshipment (VRPT)** and is decomposed into two interconnected phases:

*   **Phase 1: Collection Phase:** Determine the routes for the CVs, including their visits to SWTS for unloading.
*   **Phase 2: Transportation Phase:** Determine the routes for the TVs, ensuring their visits to SWTS are synchronized with the arrivals of CVs.

**Primary Objective:** Minimize the total number of **Collection Vehicles (CVs)** used. Minimizing Transportation Vehicles (TVs) is a secondary objective, implicitly handled by the heuristics.

## 2. Data Modeling Requirements

The following data needs to be modeled:

*   **Depot:** The starting and ending location for all CVs (1 location).
*   **Collection Points (Zones):** A set \( C \) of locations where waste needs to be collected.
    *   Each zone \( c \in C \) has coordinates.
    *   Each zone \( c \in C \) has a waste demand \( d_c \).
*   **Solid Waste Transfer Stations (SWTS):** A set \( S \) of locations where CVs unload waste and TVs load waste.
    *   Each SWTS \( s \in S \) has coordinates.
*   **Landfill:** The starting and ending location for all TVs, where waste is finally disposed (1 location).
*   **Collection Vehicles (CVs):**
    *   Homogeneous fleet (set \( K \)).
    *   Capacity \( Q \).
    *   Maximum route duration \( L \).
*   **Transportation Vehicles (TVs):**
    *   Homogeneous fleet (set \( E \)).
    *   Capacity \( Q' \).
    *   Maximum route duration \( L' \).
*   **Travel Times/Costs:** Assumed to be derived from Euclidean distances between locations (Depot, Zones, SWTS, Landfill) using a fixed speed. Loading/unloading times at zones and SWTS are considered negligible compared to travel times.

## 3. Phase 1: Collection Vehicle (CV) Routing

**Goal:** Design routes for CVs starting from the depot, visiting collection points, unloading at SWTS, potentially visiting more points, and finally having enough time to return to the depot. Minimize the number of CVs needed.

**CV Route Structure:**
*   Starts at the Depot.
*   Composed of "legs":
    *   **T1 Leg:** Depot -> Visit sequence of collection points -> End at an SWTS. The total demand collected must not exceed \( Q \).
    *   **T2 Leg:** Start at an SWTS -> Visit sequence of collection points -> End at an SWTS (can be the same or different). The total demand collected must not exceed \( Q \).
*   A complete CV route consists of exactly one T1 leg followed by zero or more T2 legs.
*   The total duration of all legs in a route (T1 + T2s) plus the time needed to return to the depot from the final SWTS must not exceed \( L \).

**Algorithms to Implement for Phase 1:**

### 3.1. Greedy Constructive Heuristic (Based on Algorithm 1 in Paper / Task a)

*   **Initialization:** Start with an empty set of routes \( K \). Initialize \( k=1 \).
*   **Loop 1 (While unassigned zones \( C \) exist):**
    *   Initialize route \( R_k \) starting at the `{depot}`.
    *   Initialize residual capacity \( q_k = Q \), residual time \( T_k = L \).
    *   **Loop 2 (While possible to add zones):**
        *   Find the collection zone \( c' \in C \) closest to the last location in \( R_k \).
        *   Calculate time \( t' \) needed to visit \( c' \), then potentially an SWTS, and then return to the depot.
        *   **Feasibility Check:** If \( d_{c'} \le q_k \) AND \( t' \le T_k \):
            *   Add \( c' \) to \( R_k \): \( R_k = R_k \cup \{c'\} \).
            *   Update \( q_k = q_k - d_{c'} \).
            *   Update \( T_k \) (reduce by travel time to \( c' \)).
            *   Remove \( c' \) from \( C \): \( C = C \setminus \{c'\} \).
        *   **Else (Infeasible):**
            *   **If infeasible due to capacity (\( d_{c'} > q_k \)) BUT time is sufficient (\( t' \le T_k \)):**
                *   Find SWTS \( s' \) closest to the last location in \( R_k \).
                *   Add \( s' \) to \( R_k \): \( R_k = R_k \cup \{s'\} \).
                *   Reset capacity: \( q_k = Q \).
                *   Update \( T_k \) (reduce by travel time to \( s' \)).
                *   (Continue Loop 2 to try adding \( c' \) or other zones again).
            *   **Else (Infeasible due to time \( t' > T_k \)):**
                *   Break Loop 2 (finalize this route).
    *   **Route Finalization:**
        *   If the last element of \( R_k \) is NOT an SWTS:
            *   Find SWTS \( s' \) closest to the last location in \( R_k \).
            *   Add \( s' \) and then `{depot}` to \( R_k \): \( R_k = R_k \cup \{s'\} \cup \{depot\} \).
        *   Else (last element IS an SWTS):
            *   Add `{depot}` to \( R_k \): \( R_k = R_k \cup \{depot\} \).
    *   Add completed route \( R_k \) to the set \( K \). Increment \( k \).
*   **Output:** Set of CV routes \( K \).

### 3.2. GRASP Constructive Phase (Task b)

*   Implement **only the constructive phase** of a GRASP algorithm.
*   This modifies the Greedy Constructive Heuristic (3.1).
*   Instead of always selecting the *closest* unassigned zone \( c' \), build a **Restricted Candidate List (RCL)** of "good" candidate zones (e.g., based on proximity).
*   **Randomly select** a zone from the RCL to add to the current route.
*   The feasibility checks (capacity, duration) and SWTS insertion logic remain similar.
*   *Note: The specific criteria for RCL construction (e.g., alpha parameter) need to be defined.*

### 3.3. Local Search Neighborhoods (for Multi-Start / GVNS) (Task c, d)

*   These operate on a given solution (set of CV routes \( K \)) for Phase 1.
*   Define and implement the following neighborhood structures for **collection routes**:
    *   **Task Reinsertion:** Move a collection zone from its current position in one route to a different position (either in the same route or a different route). Check feasibility (capacity \( Q \), duration \( L \)).
    *   **Task Exchange:** Swap the positions of two collection zones (either within the same route or between two different routes). Check feasibility.
    *   **2-opt:** Within a single CV route, reverse a segment of the route between two non-adjacent nodes (zones or SWTS) to potentially reduce travel distance/time. Check feasibility (duration \( L \)).

### 3.4. Multi-Start Method (Task c)

*   Generate multiple initial solutions for Phase 1, likely using the **GRASP Constructive Phase (3.2)**.
*   For each initial solution, apply **Local Search** using the neighborhoods defined in (3.3) until a local optimum is reached (e.g., using a Best Improvement or First Improvement strategy).
*   Keep track of the best overall solution found across all starts.

### 3.5. General Variable Neighborhood Search (GVNS) (Task d)

*   Implement GVNS for Phase 1.
*   Requires the neighborhood structures from (3.3).
*   GVNS systematically explores these neighborhoods to escape local optima. Typically involves:
    *   **Shaking:** Randomly perturb the current solution using a neighborhood \( N_k \).
    *   **Local Search:** Apply local search (e.g., Variable Neighborhood Descent - VND) using a set of neighborhoods \( N_l \) (often \( l=1...k_{max} \)) to find a local optimum from the perturbed solution.
    *   **Move Acceptance:** Decide whether to move to the new local optimum based on its quality compared to the current best.
    *   Cycle through different shaking neighborhoods (\( k=1...k_{max} \)).

## 4. Phase 2: Transportation Vehicle (TV) Routing

**Goal:** Design routes for TVs starting from the landfill, visiting SWTS to pick up waste delivered by CVs, and returning to the landfill to empty. Routes must be synchronized with CV arrival times at SWTS.

**Input:** A set \( H \) of **tasks** derived from the Phase 1 solution. Each task \( h \in H \) corresponds to a CV visiting an SWTS and is represented as a tuple \( h = (D_h, S_h, \tau_h) \):
*   \( D_h \): Amount of waste delivered by the CV.
*   \( S_h \): The specific SWTS visited.
*   \( \tau_h \): The time the CV arrives at \( S_h \).

**TV Route Structure:**
*   Starts at Landfill.
*   Visits one or more SWTS \( S_h \) to collect waste \( D_h \).
*   Returns to Landfill when full or no more tasks can be feasibly added.
*   Total route duration must not exceed \( L' \).
*   Total collected waste must not exceed \( Q' \).

**Algorithm to Implement for Phase 2:**

### 4.1. Greedy Constructive Heuristic (Based on Algorithms 3 & 4 in Paper / Task a)

*   **Initialization:**
    *   Sort the set of tasks \( H \) in ascending order of arrival time \( \tau_h \).
    *   Initialize an empty set of TV routes \( E \).
    *   Determine \( q_{min} = \min\{D_h : h \in H\} \) (minimum waste amount in a single task, used for deciding when to return to landfill).
*   **Loop (While tasks \( H \) remain):**
    *   Get the first task \( h = (D_h, S_h, \tau_h) \) from the sorted list \( H \). Remove \( h \) from \( H \).
    *   **Find Best Vehicle (Uses Algorithm 4 logic):**
        *   Initialize `best_vehicle = null`, `min_insertion_cost = infinity`.
        *   For each existing TV route \( R_e \) in \( E \):
            *   Let \( h' = (D_{h'}, S_{h'}, \tau_{h'}) \) be the last task currently assigned to \( R_e \).
            *   **Check Feasibility Conditions:**
                *   **(a) Timing:** Travel time from \( S_{h'} \) to \( S_h \) must be \( \le \tau_h - \tau_{h'} \). (If \( R_e \) is empty, this condition is met w.r.t. landfill departure time).
                *   **(b) Capacity:** Residual capacity \( q_e \ge D_h \).
                *   **(c) Duration:** The new total duration of \( R_e \) (including visiting \( S_h \) and returning to landfill) must be \( \le L' \).
            *   If feasible, calculate the insertion cost (e.g., additional travel time).
            *   If feasible and cost < `min_insertion_cost`, update `best_vehicle = e`, `min_insertion_cost = cost`.
    *   **Assign Task:**
        *   **If `best_vehicle` is `null`:**
            *   Create a new TV route \( e_{new} \). Add it to \( E \).
            *   Initialize \( R_{e_{new}} = \{landfill, S_h\} \).
            *   Initialize residual capacity \( q_{e_{new}} = Q' - D_h \).
            *   Initialize residual time \( T_{e_{new}} \) based on \( L' \) and travel time landfill -> \( S_h \).
        *   **Else (`best_vehicle` \( e \) found):**
            *   Add \( S_h \) to route \( R_e \): \( R_e = R_e \cup \{S_h\} \).
            *   Update residual capacity \( q_e = q_e - D_h \).
            *   Update residual time \( T_e \).
            *   **Check if Landfill Visit Needed:** If \( q_e < q_{min} \):
                *   Add `{landfill}` to \( R_e \): \( R_e = R_e \cup \{landfill\} \).
                *   Reset capacity \( q_e = Q' \).
                *   (Update \( T_e \) accordingly, potentially starting a new segment from landfill).
*   **Finalization:** For every route \( R_e \) in \( E \), if the last location is not `{landfill}`, add `{landfill}` to the end.
*   **Output:** Set of TV routes \( E \).

## 5. Evaluation

*   Implement the algorithms in Java or C++.
*   Test on provided instances (text file format).
*   Report results in tables (using provided templates or similar):
    *   Instance Name / #Zones
    *   Algorithm Parameters (e.g., |LRC|, kmax for GVNS)
    *   Number of CVs (#CV) - **Primary Metric**
    *   Number of TVs (#TV)
    *   CPU Time
*   Provide a LaTeX report (Overleaf) detailing the implementation (data structures, algorithms) and results.
