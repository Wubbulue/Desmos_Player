var elt = document.getElementById('calculator');
var calculator = Desmos.GraphingCalculator(elt);
// calculator.setExpression({ latex: 'y=x^2' });
// calculator.setExpression({latex: '\\left(\\left(1-t\\right)223+t224,\\left(1-t\\right)556+t562\\right)' });

console.log(window.location.href);

var expressionID = 0;

// var ws = new WebSocket(url.value);

var wsString = "ws://" + location.host + "/websocket";
console.log(wsString);
ws = new WebSocket(wsString);

ws.onopen = function () {
    console.log("websocket opened!")
};

ws.onmessage = function (ev) { 
    console.log("message recieved")
    console.log(ev)
    var split = ev.data.split("\n");

    expressionID--;

    for(expressionID;expressionID>=0;expressionID--){
        calculator.removeExpression({id:expressionID.toString()});
    }

    for(var item of split){
        calculator.setExpression({id:expressionID.toString(),latex: item,secret: true,color:Desmos.Colors.BLUE});
        expressionID++;
    }
};
ws.onerror = function (ev) {
    console.log("web socket error")
    console.log(ev)
};
ws.onclose = function () {
    ws = null;
    console.log("websocket closed")
};

function send(val){
    console.log("called send")
    if(ws.readyState==1){
        ws.send(val);
    }
};

setInterval(function(){
    send("photo");
} , 10000 );

