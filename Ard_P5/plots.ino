void cleanPlot(int y_min, int y_max, int channel) {
  // Limpa todos os 3 plots e os pontos x,y
  switch (channel) {
    case 0:
      Lcm.clearTrendCurve0();
      break;
    case 1:
      Lcm.clearTrendCurve1();
      break;
    case 2:
      Lcm.clearTrendCurve2();
      break;
  }
  xBox.write(0);
  yBox.write(0);

  // Escreve os ticks:
  ytick1.write(y_min);
  ytick2.write(y_max);
  xtick1.write(lambda_min);
  xtick2.write(lambda_min + round((lambda_max - lambda_min) / float(3)));
  xtick3.write(lambda_min + round(2 * (lambda_max - lambda_min) / float(3)));
  xtick4.write(lambda_max);

  // Limpa o label y:
  yLabel.write("", 20);
}
