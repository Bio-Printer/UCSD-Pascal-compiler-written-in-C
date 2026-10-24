PROGRAM PI;
(* Archimedes' method with DOUBLEs: Tiny-C's tests/pi.c in Pascal.
   Polygons with 6, 12, 24, ... sides inside and around a circle of
   diameter 1: their perimeters are a lower and an upper bound for pi.
   Doubling the sides:
       upper' = 2 * upper * lower / (upper + lower)   (harmonic mean)
       lower' = sqrt(upper' * lower)                  (geometric mean)
   Each step makes the gap about 4 times smaller (0.6 digits); both bounds
   close in on pi until they agree to 15 significant digits. *)
CONST TWO52 = 4503599627370496.0D0;     (* 2^52 *)
      REF = 314159265358979.0D0;        (* pi, 15 significant digits *)
TYPE DIGITROW = ARRAY [1..15] OF INTEGER;
VAR UPPER, LOWER, SIDES: DOUBLE;
    I: INTEGER;
    DONE: BOOLEAN;
    PIDIGITS: DIGITROW;

(* X rounded to a whole number (X below 2^52) *)
FUNCTION WHOLE(X: DOUBLE): DOUBLE;
BEGIN
  WHOLE := (X + TWO52) - TWO52
END;

(* the 15 digits of the whole number Y, the first one first *)
PROCEDURE SPLIT(Y: DOUBLE; VAR D: DIGITROW);
VAR Q: DOUBLE;
    J: INTEGER;
BEGIN
  FOR J := 15 DOWNTO 1 DO
    BEGIN
      Q := WHOLE(Y / 10 - 0.45);        (* Y DIV 10 *)
      D[J] := TRUNC(Y - 10 * Q);
      Y := Q
    END
END;

(* how many leading digits of X, to 14 decimals, agree with pi *)
FUNCTION DIGITS(X: DOUBLE): INTEGER;
VAR D: DIGITROW;
    N: INTEGER;
    SAME: BOOLEAN;
BEGIN
  SPLIT(WHOLE(X * 1.0D14), D);
  N := 0;
  SAME := TRUE;
  WHILE SAME AND (N < 15) DO
    IF D[N + 1] = PIDIGITS[N + 1] THEN
      N := N + 1
    ELSE
      SAME := FALSE;
  DIGITS := N
END;

BEGIN
  SPLIT(REF, PIDIGITS);
  UPPER := 2.0 * SQRT(3.0D0);           (* hexagon around the circle *)
  LOWER := 3.0;                         (* hexagon inside it *)
  SIDES := 6.0;
  WRITELN('Pi by Archimedes'' method (polygons around and inside a circle)');
  WRITELN;
  WRITELN('step       sides  lower bound         upper bound         digits');
  I := 0;
  REPEAT
    WRITELN(I:4, ' ', SIDES:11:0, '  ', LOWER:1:16, '  ', UPPER:1:16, '  ',
            DIGITS(LOWER):2, ' ', DIGITS(UPPER):2);
    DONE := (DIGITS(LOWER) >= 15) AND (DIGITS(UPPER) >= 15);
    IF NOT DONE THEN
      BEGIN
        UPPER := 2.0 * UPPER * LOWER / (UPPER + LOWER);
        LOWER := SQRT(UPPER * LOWER);
        SIDES := SIDES * 2.0
      END;
    I := I + 1
  UNTIL DONE OR (I > 30);
  WRITELN;
  WRITELN('pi = ', (LOWER + UPPER) / 2.0:1:14, ' (atan: ', 4.0 * ATAN(1.0D0):1:14, ')')
END.
