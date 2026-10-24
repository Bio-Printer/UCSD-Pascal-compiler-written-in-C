PROGRAM DOUBLES;
(* DOUBLE (IEEE binary64, the emulator's CSP 100..137): constants,
   arithmetic, conversions, comparisons, the standard functions, WRITE,
   parameters, function results, arrays and records. *)
CONST TENTH = 0.1;                      (* a REAL constant: a DOUBLE where it meets one *)
      BIG = 1.0D300;
      NEGD = -2.5D0;
      NEGT = -TENTH;
TYPE POINT = RECORD X, Y: DOUBLE; TAG: INTEGER END;
VAR A, B, C: DOUBLE;
    R: REAL;
    I, J: INTEGER;
    V: ARRAY [1..3] OF DOUBLE;
    P: POINT;
    BOOL: BOOLEAN;

FUNCTION HYPOT(X, Y: DOUBLE; N: INTEGER): DOUBLE;
BEGIN
  HYPOT := SQRT(X * X + Y * Y) + N
END;

PROCEDURE SWAP(VAR X, Y: DOUBLE);
VAR T: DOUBLE;
BEGIN
  T := X; X := Y; Y := T
END;

PROCEDURE OUTER;
VAR S: DOUBLE;
  PROCEDURE INNER;
  BEGIN
    S := S * 2 + 0.25
  END;
BEGIN
  S := 1;
  INNER;
  INNER;
  WRITELN('nested ', S:1:4)
END;

PROCEDURE CONSTS;
BEGIN
  WRITELN('constants');
  A := TENTH;
  R := TENTH;
  B := R;
  WRITELN(A:1:17, ' ', A * 3:1:17, ' ', TENTH * A:1:17, ' ', B:1:17);
  A := NEGT;
  B := -0.5;
  WRITELN(NEGD:1:3, ' ', A:1:17, ' ', B:1:3, ' ', BIG, ' ', -BIG:1:0);
  A := -0.1;
  B := 1.5D-3;
  WRITELN(A:1:17, ' ', B:1:6, ' ', 2D0:1:1, ' ', 1D3:1:1)
END;

PROCEDURE ARITH;
BEGIN
  WRITELN('arithmetic');
  A := 7.5; B := 2;
  WRITELN(A + B:1:4, ' ', A - B:1:4, ' ', A * B:1:4, ' ', A / B:1:4, ' ', -A:1:4);
  I := 3; R := 0.5;
  WRITELN(I + A:1:4, ' ', A - I:1:4, ' ', I / A:1:6, ' ', R * A:1:4, ' ', A / R:1:4);
  WRITELN(1 / 3.0D0:1:16, ' ', 2 - A * (I + 1):1:3, ' ', (A + 1) * (B - 1) / 4:1:5);
  C := 1;
  FOR J := 1 TO 10 DO C := C * 10 + J;
  WRITELN(C:1:1)
END;

PROCEDURE COMPARE;
BEGIN
  WRITELN('comparisons');
  A := 1.5; B := 2.5;
  WRITELN(ORD(A < B), ORD(A <= B), ORD(A > B), ORD(A >= B), ORD(A = B), ORD(A <> B));
  WRITELN(ORD(B < A), ORD(B <= A), ORD(B > A), ORD(B >= A), ORD(A = A), ORD(A <> A));
  BOOL := A > 1.4999999999;
  WRITELN(ORD(A = 1.5), ORD(1 < A), ORD(BOOL), ORD(TENTH = 0.1D0), ORD(R < B), ORD(SNGL(A) = 1.5))
END;

PROCEDURE FUNCS;
BEGIN
  WRITELN('functions');
  A := 2;
  WRITELN(SQRT(A):1:15, ' ', SQR(A + 1):1:1, ' ', ABS(-A):1:1, ' ', ABS(NEGD):1:2);
  WRITELN(SIN(0.5D0):1:15, ' ', COS(0.5D0):1:15, ' ', ATAN(1D0) * 4:1:15);
  WRITELN(EXP(1D0):1:15, ' ', LN(10D0):1:15, ' ', LOG(2D0):1:15);
  WRITELN(TRUNC(2.7D0), ' ', TRUNC(-2.7D0), ' ', ROUND(2.5D0), ' ', ROUND(-2.5D0), ' ',
          ROUND(2.4999D0), ' ', ROUND(-0.4D0));
  R := SNGL(1.25D0);
  A := R;
  WRITELN(A:1:3);
  R := SNGL(3);
  A := R;
  WRITELN(A:1:1)
END;

PROCEDURE WRITES;
BEGIN
  WRITELN('write');
  A := 3.14159265358979D0;
  WRITELN(A);
  WRITELN(A:25);
  WRITELN(A:8:3, '|', A:1:2, '|', -A:10:4, '|', A:0:0);
  WRITELN(1.0D-20, ' ', -6.02214076D23:12)
END;

PROCEDURE STRUCTS;
BEGIN
  WRITELN('records, arrays, parameters');
  V[1] := 1; V[2] := 2; V[3] := V[1] + V[2];
  P.X := 3; P.Y := 4; P.TAG := 7;
  WRITELN(V[3]:1:1, ' ', HYPOT(P.X, P.Y, P.TAG):1:1, ' ', HYPOT(5, 12, 0):1:1);
  SWAP(V[1], V[3]);
  WRITELN(V[1]:1:1, ' ', V[3]:1:1);
  OUTER
END;

BEGIN
  CONSTS;
  ARITH;
  COMPARE;
  FUNCS;
  WRITES;
  STRUCTS;
  WRITELN('done')
END.
