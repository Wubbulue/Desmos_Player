var elt = document.getElementById('calculator');
var calculator = Desmos.GraphingCalculator(elt);
// calculator.setExpression({ latex: 'y=x^2' });
// calculator.setExpression({latex: '\\left(\\left(1-t\\right)223+t224,\\left(1-t\\right)556+t562\\right)' });

console.log(window.location.href);

var expressionID = 0;
var frame = 0;

// var ws = new WebSocket(url.value);

var wsString = "ws://" + location.host + "/websocket";
console.log(wsString);

var timer;


ws = new WebSocket(wsString);

ws.onopen = function () {
    console.log("websocket opened!")
};

function updateFrameCounter(){
}

ws.onmessage = function (ev) { 

    //our packet is delimted by $ [ended, time in S, frame number, latex ]
    const args = ev.data.split("$");
    console.log(args);



    if(args[0]!="0"){
        stop();
        return;
    }

    var split = args[3].split("\n");
    console.log(split.length);


    var tmpState = calculator.getState();
    //make sure the id is different for each expression
    tmpState.expressions.list = [];

    tmpState.expressions.list.push({ type: "text", text: "Frame count: " + args[2] });
    tmpState.expressions.list.push({ type: "text", text: "Video time: " + args[1] });

    for(var item of split){
        // calculator.setExpression({latex: item,secret: true,color:Desmos.Colors.BLUE});
        tmpState.expressions.list.push({type:"expression",latex: item,secret: true,color:Desmos.Colors.BLUE});
        // expressionID++;
    }

    calculator.setState(tmpState);

    frame++;

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

function start(){
    timer = setInterval(function () {
        send("photo");
    }, 3500);
};

function stop() {
    clearInterval(timer);

    var tmpState = calculator.getState();
    //make sure the id is different for each expression

    tmpState.expressions.list.push({ type: "text", text: "Video ended!"});

    calculator.setState(tmpState);

}

function screenshot(){
    html2canvas(document.body).then(function(canvas) {

        var dataURL = canvas.toDataURL("image/png");
        var data = atob(dataURL.substring("data:image/png;base64,".length)),
            asArray = new Uint8Array(data.length);

        for (var i = 0, len = data.length; i < len; ++i) {
            asArray[i] = data.charCodeAt(i);
        }

        var blob = new Blob([asArray.buffer], { type: "image/png" });

        send(blob);

    });
};

