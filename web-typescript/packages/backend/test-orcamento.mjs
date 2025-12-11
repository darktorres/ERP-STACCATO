async function testOrcamentoApi() {
  try {
    // Test 1: List all orcamentos without filters
    console.log('\n=== Test 1: List all orcamentos ===');
    const response = await fetch('http://localhost:3001/trpc/orcamento.list', {
      method: 'POST',
      headers: {
        'Content-Type': 'application/json',
        'Authorization': 'Bearer eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9.eyJpZFVzdWFyaW8iOjEsInRpcG8iOiJBRE1JTklTVFJBRE9SIiwiaWRMb2phIjoxfQ.test'
      },
      body: JSON.stringify({
        jsonrpc: '2.0',
        id: 1,
        method: 'orcamento.list',
        params: { input: {} }
      })
    });

    const data = await response.json();
    console.log('Response status:', response.status);
    console.log('Full response:', JSON.stringify(data, null, 2));

    if (data.result) {
      console.log(`\nFound ${data.result.length} orcamentos`);
      if (data.result.length > 0) {
        console.log('First orcamento:', JSON.stringify(data.result[0], null, 2));
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
