module top (
    in1,
    in2,
    in3,
    clk1,
    clk2,
    clk3,
    clk4,
    clk5,
    out1,
    out2
);
  input in1, in2, in3;
  input clk1, clk2, clk3, clk4, clk5;
  output out1, out2;
  wire r1q, r2q, r3q, r4d, r5d;
  wire s1, s2, s3, s4, s5;

  DFF_X1 r1 (
      .D (in1),
      .CK(clk1),
      .Q (r1q)
  );
  DFF_X1 r2 (
      .D (in2),
      .CK(clk2),
      .Q (r2q)
  );
  DFF_X1 r3 (
      .D (in3),
      .CK(clk3),
      .Q (r2q)
  );

  // SUM = A ^ B ^ CIN
  XOR2_X1 x1 (
      .Z(s1),
      .A(r1q),
      .B(r2q)
  );
  XOR2_X1 x2 (
      .Z(r4d),
      .A(s1),
      .B(r3q)
  );
  // COUT = A & B | A & CIN | B & CIN
  AND2_X1 a1 (
      .ZN(s2),
      .A1(r1q),
      .A2(r2q)
  );
  AND2_X1 a2 (
      .ZN(s3),
      .A1(r1q),
      .A2(r3q)
  );
  AND2_X1 a3 (
      .ZN(s4),
      .A1(r2q),
      .A2(r3q)
  );
  OR2_X1 O1 (
      .ZN(s5),
      .A1(s2),
      .A2(s3)
  );
  OR2_X1 O2 (
      .ZN(r5d),
      .A1(s4),
      .A2(s5)
  );

  DFF_X1 r4 (
      .D (r4d),
      .CK(clk4),
      .Q (out1)
  );
  DFF_X1 r5 (
      .D (r5d),
      .CK(clk5),
      .Q (out2)
  );
endmodule
