
    function sendButton(Pin){
		server = '/?state=' + Pin;
		request = new XMLHttpRequest();
		request.open('GET', server, true);
		request.send();
    }
	
	/*function saveButton(var nameURL){
		var login = document.getElementById('login');
		var password = document.getElementById('password');
		server = nameURL + '?login=' + login + '&password=' + password;
		request = new XMLHttpRequest();
		request.open('GET', server, true);
		request.send();
    }*/
	
	function saveButton(var URL){
		var login = document.getElementById('login');
		var password = document.getElementById('password');
		var data = {ssid:login, password:password};
		
		var request = new XMLHttpRequest();
		
		request.open('POST', URL, true);
		request.send(JSON.stringify(data));
	}
	
	
    function updateData(){
		fetch('/getApiData')
			.then((data) => {
				return data.text();
			})
			.then((data) => {
				str = '';
				if(data == 'True'){
					str = 'Now opened';
					document.getElementById('status').innerHTML = str;
				} else{
					str = 'Now closed';
					document.getElementById('status').innerHTML = str;
				}
				setTimeout(updateData, 10000);
			})
			.catch((err) => {
				console.log('error');
				setTimeout(updateData, 10000);
		});
    }
	
    updateData();