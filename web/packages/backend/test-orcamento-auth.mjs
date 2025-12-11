async function testOrcamentoApi() {
  try {
    // Step 1: Login
    console.log('\n=== Step 1: Login ===');
    const loginResponse = await fetch('http://localhost:3001/trpc/auth.login', {
      method: 'POST',
      headers: {
        'Content-Type': 'application/json',
      },
      body: JSON.stringify({
        jsonrpc: '2.0',
        id: 1,
        method: 'auth.login',
        params: { 
          input: {
            user: 'torres',
            password: 'torres123',
            staging: false
          }
        }
      })
    });

    const loginData = await loginResponse.json();
    console.log('Login response:', JSON.stringify(loginData, null, 2));

    if (!loginData.result || !loginData.result.token) {
      console.error('Login failed');
      process.exit(1);
    }

    const token = loginData.result.token;
    console.log('Got token:', token.substring(0, 20) + '...');

    // Step 2: List orcamentos
    console.log('\n=== Step 2: List all orcamentos ===');
    const response = await fetch('http://localhost:3001/trpc/orcamento.list', {
      method: 'POST',
      headers: {
        'Content-Type': 'application/json',
        'Authorization': `Bearer ${token}`
      },
      body: JSON.stringify({
        jsonrpc: '2.0',
        id: 2,
        method: 'orcamento.list',
        params: { input: {} }
      })
    });

    const data = await response.json();
    console.log('Response status:', response.status);
    
    if (data.result) {
      console.log(`\nFound ${data.result.length} orcamentos`);
      if (data.result.length > 0) {
        console.log('First orcamento:', JSON.stringify(data.result[0], null, 2));
      } else {
        console.log('No orcamentos returned!');
      }
    } else if (data.error) {
      console.log('Error:', JSON.stringify(data.error, null, 2));
    }

  } catch (error) {
    console.error('Test failed:', error.message);
  }

  process.exit(0);
}

testOrcamentoApi();
