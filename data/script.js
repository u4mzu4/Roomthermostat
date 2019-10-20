var p2 = document.getElementById('parameter-2');
var submit_btn = document.getElementById('submit-btn');
var topSlider = document.getElementById('myRange');
var top_slider_current = document.getElementById('top-slider-current');
var bottomSlider = document.getElementById('myRange2');
var bottom_slider_current = document.getElementById('bottom-slider-current');

top_slider_current.innerHTML = `Actual Value: ${topSlider.value}`;

/* Event Handler for top slider */
topSlider.addEventListener('change', function () {
    top_slider_current.innerHTML = `Actual Value: ${topSlider.value}`;
});

bottom_slider_current.innerHTML = `Actual Value: ${bottomSlider.value}`;

/* Event Handler for bottom slider */

bottomSlider.addEventListener('change', function () {
    bottom_slider_current.innerHTML = `Actual Value: ${bottomSlider.value}`;
});

var top_btn_state = false;
p2.addEventListener('change', function (e) {
    top_btn_state = e.target.state;
});



/* Dynamic input programming */

/* Circular guage */

var data_value = document.getElementById('data-value');
var data_show = document.getElementById('data');
data_show.innerHTML = data_value.value;
data_value.addEventListener('change', function () {
    data_show.innerHTML = data_value.value;
});

/* Indicator LED Lights functionality */

//hidden input fields selection
var radiator = document.getElementById('radiator');
var kazan = document.getElementById('kazan');
var paldo = document.getElementById('paldo');

//led selection
var radiator_led = document.getElementById('radiator-led');
var kazan_led = document.getElementById('kazan-led');
var paldo_led = document.getElementById('paldo-led');


//radiator funcionality
if (radiator.value == 'on') {
    radiator_led.className += ' btn-02-on';
    radiator_led.value = 'BE';
}
else {
    radiator_led.classList.remove('btn-02-on');
}

//kazan functionality
if (kazan.value == 'on') {
    kazan_led.className += ' btn-02-on';
    kazan_led.value = 'BE';
}
else {
    kazan_led.classList.remove('btn-02-on');
}

//paldo functionality
if (paldo.value == 'on') {
    paldo_led.className += ' btn-02-on';
    paldo_led.value = 'BE';
}
else {
    paldo_led.classList.remove('btn-02-on');
}

//Making Query string

var p1 = document.getElementsByName('p1');
var p2 = document.getElementsByName('p2');
var p3 = document.getElementsByName('p3');
var p5 = document.getElementsByName('p5');

//default states of parameter 2 and parameter 5
var p2State = 0;
var p5State = 1;

submit_btn.addEventListener('click', function () {
    //checking the checked value for p2
    if (p2[0].checked) {
        p2State = 1;
    }
    else {
        p2State = 0;
    }

    //checking the checked value for p5
    if (p5[1].checked) {
        p5State = 2;
    }
    else {
        p5State = 1;
    }

    history.pushState({
        id: 'homepage'
    },
        'new', '?p1=' + p1[0].value + '&p2=' + p2State + '&p3=' + p3[0].value + '&p4=' + p5State);
    location.reload(true);
});

