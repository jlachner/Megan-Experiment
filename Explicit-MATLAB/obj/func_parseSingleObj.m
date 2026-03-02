function data = func_parseSingleObj( name, colorFlag )

% dir_name: folder relative to
% nq: the number of segments
% quality: either 'highQuality' or 'lowQuality'
% name: the number that should be saved

% For our internal use, the relative directory will be fixed
dir_name = './obj/';

% Read the file named as        : dir_name/
file_name = [ dir_name, '/object.obj' ];

% Each line of .obj file consists as follows:
% h: (1x6) array of integers
% v: (1x3) array of Vertices
% f: (1x6) array of Faces, first three is the 3D position,
%                           last three is the color.
% Here, we are only interested reading the

% The maximum number of samples
Nmax = 2^16;

tmpv = zeros( Nmax, 3 );
if colorFlag
    tmpf = zeros( Nmax, 6 );
else
    tmpf = zeros( Nmax, 3 );
end

nv = 1;
nf = 1;

% Open the .obj file and create a file ID
fid = fopen( file_name );

% We read line-by-line and parse each sentence.
tline = fgetl( fid );

while ischar( tline )

    % The first character is either 'h', 'v', 'f'.
    assert( any( strcmp( tline( 1 ), { 'h', 'v', 'f' } ) ) );

    % If exists, then read the numbers and make it as an array
    % The numbers are first saved as a cell array

    % Change the cell array to numbers, and attach it as an array
    switch tline( 1 )
        case 'v'
            tmpv( nv, : ) = str2num( tline( 2 : end ) ) ;
            nv = nv + 1;
        case 'f'
            tmpf( nf, : ) = str2num( tline( 2 : end ) ) ;
            nf = nf + 1;
        otherwise
    end

    % Renew the line
    tline = fgetl( fid );

end

fclose( fid );

data{ i } = struct( 'v', tmpv( 1:nv-1, : ), 'f', tmpf( 1:nf-1, : ) );


% Safe as .mat file
save( ['./graphics/', name, '.mat'] , 'data' );

end