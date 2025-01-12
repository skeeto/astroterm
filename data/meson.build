project_source_files += [
    files('keplerian_elements.c'),
]

# Verify bsc5 downloaded correctly
bsc5_path = 'bsc5'
expected_hash = 'e471d02eaf4eecb61c12f879a1cb6432ba9d7b68a9a8c5654a1eb42a0c8cc340'
computed_hash = fs.hash(bsc5_path, 'sha256')

if computed_hash != expected_hash
  error('bsc5 SHA-256 hash mismatch! Build halted.\n' +
        'Expected:\t@0@\n'.format(expected_hash) +
        'Got:\t\t@0@'.format(computed_hash))
else
  message('bsc5 SHA-256 hash verified successfully: @0@'.format(computed_hash))
endif
