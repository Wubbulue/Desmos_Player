var elt = document.getElementById('calculator');
var calculator = Desmos.GraphingCalculator(elt);
calculator.setExpression({ id: 'graph1', latex: 'y=x^2' });
calculator.setExpression({ id: 'graph2', latex: 'y=x^3' });

var index = 1;
