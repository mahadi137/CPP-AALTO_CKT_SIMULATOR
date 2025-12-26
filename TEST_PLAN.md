# Circuit Simulator - Component Node Synchronization Test Plan

## Overview

Test the automatic component node synchronization system that updates component node numbers based on wire-carried node positions.

## Test Scenarios

### Test 1: Basic Component Movement and Wire Snapping

**Steps:**

1. Create 2 resistors (R1, R2) separated on canvas
2. Create a wire connecting R1's node B to R2's node A
3. Move R1 or R2 alone - should keep its node numbers
4. Move one component toward a wire - should adopt wire's node number when close
5. Move component away from wire - should revert to original node number

**Expected Result:**

- Component node numbers only change when position matches a wire-carry node position
- Node numbers remain constant when not near wires

---

### Test 2: Complete Circuit Movement

**Steps:**

1. Create a circuit: V1 connected to R1 and R2 in series
2. Select entire circuit (all 3 components + all wires)
3. Drag entire circuit to new location
4. Release mouse

**Expected Result:**

- All component and wire positions update
- Component node numbers remain unchanged during movement
- After release, node numbers stay the same (no involuntary syncs)
- Circuit topology preserved

---

### Test 3: Component Deletion

**Steps:**

1. Create circuit: V1 → R1 → R2 → back to V1
2. Delete R1
3. Check remaining components (V1, R2) node numbers

**Expected Result:**

- R1 is deleted
- V1 node numbers unchanged
- R2 node numbers unchanged
- Circuit updates correctly with deleted component removed

---

### Test 4: Wire Connection and Node Merging

**Steps:**

1. Create 2 disconnected resistors: R1 (nodes 1,2) and R2 (nodes 3,4)
2. Draw wire from R1 node B (node 2) to R2 node A (node 3)
3. Complete wire with ESC

**Expected Result:**

- Nodes properly merge (e.g., node 3 → node 2)
- Wire-carry nodes are cleaned up
- Circuit shows merged connection
- R2 shows nodes [2,4] (merged node and original node B)

---

### Test 5: Component Snapping to Existing Wire

**Steps:**

1. Create circuit: V1 → R1 (with wire between them)
2. Create R2 unconnected (nodes 5,6)
3. Draw wire from R2 node A (node 5) to the existing wire between V1 and R1
4. Complete wire with ESC

**Expected Result:**

- R2 node A adopts the wire's node number (should merge to V1's node)
- R2 now properly connected in circuit
- Node merge visible in component properties
- No orphaned nodes

---

### Test 6: Movement Triggers Sync (Optional)

**Steps:**

1. Create circuit with wire node 1 at position (600, 400)
2. Create component at separate location with nodes 5,6
3. Drag component to position (600, 400)
4. Release mouse

**Expected Result:**

- Component positions update
- After release, component node A syncs to wire's node 1
- Component now shows adopted node number from wire

---

### Test 7: Wire-Only Movement

**Steps:**

1. Create circuit with connected components
2. Select only wires (not components)
3. Drag wires to new location
4. Release mouse

**Expected Result:**

- Wire visual positions update
- Component node numbers stay the same
- No unintended syncs triggered
- Circuit topology unchanged

---

## Verification Checklist

- [ ] Positions update correctly for moved items
- [ ] Node numbers change only when warranted (wire snapping)
- [ ] Node numbers don't change during normal movement
- [ ] Wire connections properly merge nodes
- [ ] Stale wire-carry nodes don't cause issues
- [ ] Deletion doesn't affect remaining components
- [ ] Complete circuit moves as a unit
- [ ] No orphaned nodes after connections
- [ ] Circuit topology always preserved

---

## Key Behaviors to Confirm

1. **Movement Only**: Positions change, node numbers stay same
2. **Wire Snapping**: Position matches wire → adopt wire's node
3. **Wire Connection**: Nodes properly merge, stale data cleaned
4. **Deletion**: Other components unaffected
5. **Sync Timing**: Wire positions update BEFORE component sync
