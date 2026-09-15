# Documentation-first development

## Rule

Product behavior, UX semantics and architectural boundaries are changed in canonical documentation before implementation. Code reviews must be able to point from an implementation behavior to its governing contract.

## Change sequence

1. change/approve product or UX contract;
2. update architecture if ownership/boundaries change;
3. update capability status from the new claim as `Specified`/`Unproven` as appropriate;
4. plan or update the implementation gate in `infra-planner`;
5. implement only the selected gate;
6. collect automated and, when required, physical evidence;
7. update capability status to the evidence-backed state.

## UX artifacts

Exact LCD layouts are treated as specifications, not comments in C. Screen artifacts must preserve the 9-row/21-character contract and may be paired with machine-readable metadata for actions, current-state conditions and hint bindings. A generator/validator may produce C tables from those specs, but generated code is not the canonical copy.

## Gate discipline

Implementation proceeds one gate at a time. Physical conclusions are never inferred from compilation or CI. A gate requiring hardware remains incomplete until the operator provides the required physical evidence.

The implementation plan lives in `tiagooliveirajs/infra-planner`; this repository contains product/architecture truth and product-specific evidence/code.
