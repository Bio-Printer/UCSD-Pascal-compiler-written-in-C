PROGRAM READTEST;
(* READ of a DOUBLE: from the console, and from a text file it writes *)
VAR F: TEXT;
    A, B, C: DOUBLE;
    I: INTEGER;

PROCEDURE CONSOLE;
BEGIN
  WRITE('numbers: ');
  READ(A, B);
  READLN(C);
  WRITELN(A:1:3, ' ', B:1:3, ' ', C:1:3, ' ', A + B + C:1:3)
END;

PROCEDURE WRITEFILE;
BEGIN
  REWRITE(F, 'DBL.TEXT');
  WRITELN(F, 0.1D0, ' ', -2.5D-7:1:9, ' ', 12345);
  WRITELN(F, '  6.02214076E23x');
  CLOSE(F, LOCK)
END;

PROCEDURE READFILE;
BEGIN
  RESET(F, 'DBL.TEXT');
  READ(F, A, B, I);
  READLN(F);
  READ(F, C);
  WRITELN(A:1:17, ' ', B:1:12, ' ', I, ' ', C, ' ', F^);
  CLOSE(F)
END;

BEGIN
  CONSOLE;
  WRITEFILE;
  READFILE;
  WRITELN(ORD(A = 0.1D0), ORD(B = -2.5D-7), ORD(C = 6.02214076D23))
END.
